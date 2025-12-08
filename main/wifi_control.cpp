#include "wifi_control.h"
#include "mode_manager.h"
#include "pid_controller.h"
#include "ble_control.h"
#include <WebSocketsServer.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// WebSocket Server on port 81
static WebSocketsServer webSocket(81);

// HTTP Server on port 80 (for serving web page)
static WebServer httpServer(HTTP_SERVER_PORT);
static bool serverRunning = false;

// Telemetry data
static double currentAngle = 0;
static double currentOutput = 0;
static unsigned long lastUpdateTime = 0;

// Connected clients count
static uint8_t connectedClients = 0;

// Forward declarations
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
void handleRoot();
void processWebSocketMessage(uint8_t num, uint8_t *payload, size_t length);

void wifiServerInit()
{
    // Setup WebSocket server
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);

    // Setup HTTP server for serving the web page
    httpServer.on("/", HTTP_GET, handleRoot);
    httpServer.begin();

    DEBUG_PRINTLN(F("WebSocket server initialized on port 81"));
    DEBUG_PRINTLN(F("HTTP server initialized on port 80"));
}

void wifiServerStart()
{
    if (serverRunning)
    {
        return;
    }

    serverRunning = true;
    DEBUG_PRINTLN(F("WebSocket server started on port 81"));
}

void wifiServerStop()
{
    if (!serverRunning)
    {
        return;
    }

    webSocket.disconnect();
    serverRunning = false;

    DEBUG_PRINTLN(F("WebSocket server stopped"));
}

void wifiServerHandle()
{
    if (serverRunning)
    {
        webSocket.loop();
        httpServer.handleClient();
    }
}

void wifiSendTelemetry(double angle, double output)
{
    currentAngle = angle;
    currentOutput = output;
    lastUpdateTime = millis();

    // Broadcast telemetry to all connected clients
    if (connectedClients > 0)
    {
        StaticJsonDocument<200> doc;
        doc["type"] = "telemetry";
        doc["angle"] = angle;
        doc["output"] = output;
        doc["timestamp"] = lastUpdateTime;

        double kp, ki, kd, setpoint;
        pidGetValues(&kp, &ki, &kd, &setpoint);
        doc["setpoint"] = setpoint;

        String message;
        serializeJson(doc, message);
        webSocket.broadcastTXT(message);
    }
}

void wifiBroadcast(const String &message)
{
    if (connectedClients > 0)
    {
        webSocket.broadcastTXT(message);
    }
}

bool wifiHasClients()
{
    return connectedClients > 0;
}

// ============================================================================
// WebSocket Event Handler
// ============================================================================

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
    switch (type)
    {
    case WStype_DISCONNECTED:
        DEBUG_PRINTF("[WS] Client %u disconnected\n", num);
        if (connectedClients > 0)
            connectedClients--;
        break;

    case WStype_CONNECTED:
    {
        DEBUG_PRINTF("[WS] Client %u connected\n", num);
        connectedClients++;

        // Send initial status to new client
        StaticJsonDocument<300> doc;
        doc["type"] = "status";
        doc["mode"] = modeGetName(modeGetCurrent());
        doc["mode_id"] = (int)modeGetCurrent();
        doc["angle"] = currentAngle;
        doc["output"] = currentOutput;
        doc["uptime"] = millis() / 1000;

        double kp, ki, kd, setpoint;
        pidGetValues(&kp, &ki, &kd, &setpoint);
        doc["kp"] = kp;
        doc["ki"] = ki;
        doc["kd"] = kd;
        doc["setpoint"] = setpoint;

        String message;
        serializeJson(doc, message);
        webSocket.sendTXT(num, message);
    }
    break;

    case WStype_TEXT:
        processWebSocketMessage(num, payload, length);
        break;

    case WStype_PING:
        // Pong is sent automatically
        break;

    case WStype_PONG:
        break;

    default:
        break;
    }
}

// ============================================================================
// Process incoming WebSocket messages
// ============================================================================

void processWebSocketMessage(uint8_t num, uint8_t *payload, size_t length)
{
    StaticJsonDocument<300> doc;
    DeserializationError error = deserializeJson(doc, payload, length);

    if (error)
    {
        webSocket.sendTXT(num, "{\"type\":\"error\",\"message\":\"Invalid JSON\"}");
        return;
    }

    String msgType = doc["type"] | "";

    // Handle command messages
    if (msgType == "command")
    {
        String cmdStr = doc["command"] | "";
        int speed = doc["speed"] | 200;

        RobotCommand cmd = CMD_NONE;

        if (cmdStr == "forward")
            cmd = CMD_FORWARD;
        else if (cmdStr == "backward")
            cmd = CMD_BACKWARD;
        else if (cmdStr == "left")
            cmd = CMD_LEFT;
        else if (cmdStr == "right")
            cmd = CMD_RIGHT;
        else if (cmdStr == "stop")
            cmd = CMD_STOP;
        else
        {
            webSocket.sendTXT(num, "{\"type\":\"error\",\"message\":\"Unknown command\"}");
            return;
        }

        // Send to command queue
        if (xCommandQueue != NULL)
        {
            CommandMessage msg;
            msg.command = cmd;
            msg.speed = speed;
            msg.timestamp = millis();

            if (xQueueSend(xCommandQueue, &msg, pdMS_TO_TICKS(100)) == pdTRUE)
            {
                webSocket.sendTXT(num, "{\"type\":\"ack\",\"status\":\"ok\"}");
            }
            else
            {
                webSocket.sendTXT(num, "{\"type\":\"error\",\"message\":\"Queue full\"}");
            }
        }
    }
    // Handle PID update messages
    else if (msgType == "pid")
    {
        double kp, ki, kd, setpoint;
        pidGetValues(&kp, &ki, &kd, &setpoint);

        if (doc.containsKey("kp"))
            kp = doc["kp"];
        if (doc.containsKey("ki"))
            ki = doc["ki"];
        if (doc.containsKey("kd"))
            kd = doc["kd"];
        if (doc.containsKey("setpoint"))
        {
            setpoint = doc["setpoint"];
            pidSetSetpoint(setpoint);
        }

        pidSetTunings(kp, ki, kd);

        // Send updated values back
        StaticJsonDocument<200> response;
        response["type"] = "pid_updated";
        response["kp"] = kp;
        response["ki"] = ki;
        response["kd"] = kd;
        response["setpoint"] = setpoint;

        String respStr;
        serializeJson(response, respStr);
        webSocket.sendTXT(num, respStr);
    }
    // Handle status request
    else if (msgType == "get_status")
    {
        StaticJsonDocument<300> response;
        response["type"] = "status";
        response["mode"] = modeGetName(modeGetCurrent());
        response["mode_id"] = (int)modeGetCurrent();
        response["angle"] = currentAngle;
        response["output"] = currentOutput;
        response["uptime"] = millis() / 1000;

        double kp, ki, kd, setpoint;
        pidGetValues(&kp, &ki, &kd, &setpoint);
        response["kp"] = kp;
        response["ki"] = ki;
        response["kd"] = kd;
        response["setpoint"] = setpoint;

        String respStr;
        serializeJson(response, respStr);
        webSocket.sendTXT(num, respStr);
    }
    // Handle get PID request
    else if (msgType == "get_pid")
    {
        double kp, ki, kd, setpoint;
        pidGetValues(&kp, &ki, &kd, &setpoint);

        StaticJsonDocument<200> response;
        response["type"] = "pid";
        response["kp"] = kp;
        response["ki"] = ki;
        response["kd"] = kd;
        response["setpoint"] = setpoint;

        String respStr;
        serializeJson(response, respStr);
        webSocket.sendTXT(num, respStr);
    }
}

// ============================================================================
// HTTP Handler (serves the web page)
// ============================================================================

void handleRoot()
{
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 Robot - WebSocket Control</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial; background: #1a1a2e; color: #fff; text-align: center; padding: 20px; }
        h1 { color: #00d9ff; }
        .status { background: rgba(255,255,255,0.1); padding: 15px; border-radius: 10px; margin: 20px auto; max-width: 400px; }
        .connected { color: #00ff88; }
        .disconnected { color: #ff4444; }
        .btn { background: #00d9ff; border: none; color: #000; padding: 20px 30px; margin: 5px; border-radius: 10px; font-size: 18px; cursor: pointer; }
        .btn:active { background: #00ff88; }
        .btn-stop { background: #ff4444; color: #fff; }
        .controls { margin: 20px 0; }
        .telemetry { display: flex; justify-content: center; gap: 30px; margin: 20px 0; }
        .telem-item { text-align: center; }
        .telem-value { font-size: 2em; color: #00ff88; }
        .telem-label { color: #888; font-size: 0.9em; }
    </style>
</head>
<body>
    <h1>ESP32 Self-Balancing Robot</h1>
    <div class="status">
        <span id="wsStatus" class="disconnected">Disconnected</span>
    </div>
    <div class="telemetry">
        <div class="telem-item"><div class="telem-value" id="angle">--</div><div class="telem-label">Angle</div></div>
        <div class="telem-item"><div class="telem-value" id="output">--</div><div class="telem-label">Output</div></div>
        <div class="telem-item"><div class="telem-value" id="setpoint">--</div><div class="telem-label">Setpoint</div></div>
    </div>
    <div class="controls">
        <div><button class="btn" onmousedown="send('forward')" onmouseup="send('stop')" ontouchstart="send('forward')" ontouchend="send('stop')">&#9650;</button></div>
        <div>
            <button class="btn" onmousedown="send('left')" onmouseup="send('stop')" ontouchstart="send('left')" ontouchend="send('stop')">&#9664;</button>
            <button class="btn btn-stop" onclick="send('stop')">STOP</button>
            <button class="btn" onmousedown="send('right')" onmouseup="send('stop')" ontouchstart="send('right')" ontouchend="send('stop')">&#9654;</button>
        </div>
        <div><button class="btn" onmousedown="send('backward')" onmouseup="send('stop')" ontouchstart="send('backward')" ontouchend="send('stop')">&#9660;</button></div>
    </div>
    <script>
        var ws;
        function connect() {
            ws = new WebSocket('ws://' + location.hostname + ':81');
            ws.onopen = function() {
                document.getElementById('wsStatus').textContent = 'Connected';
                document.getElementById('wsStatus').className = 'connected';
            };
            ws.onclose = function() {
                document.getElementById('wsStatus').textContent = 'Disconnected';
                document.getElementById('wsStatus').className = 'disconnected';
                setTimeout(connect, 2000);
            };
            ws.onmessage = function(e) {
                var data = JSON.parse(e.data);
                if (data.angle !== undefined) document.getElementById('angle').textContent = data.angle.toFixed(1);
                if (data.output !== undefined) document.getElementById('output').textContent = data.output.toFixed(0);
                if (data.setpoint !== undefined) document.getElementById('setpoint').textContent = data.setpoint.toFixed(1);
            };
        }
        function send(cmd) {
            if (ws && ws.readyState === WebSocket.OPEN) {
                ws.send(JSON.stringify({type: 'command', command: cmd}));
            }
        }
        connect();
    </script>
</body>
</html>
)rawliteral";

    httpServer.send(200, "text/html", html);
}
