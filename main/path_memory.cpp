#include "path_memory.h"
#include "ble_control.h"
#include <ArduinoJson.h>

// Path storage
static PathPoint pathPoints[MAX_PATH_POINTS];
static int pathPointCount = 0;
static int playbackIndex = 0;

// State
static PathState currentState = PATH_IDLE;
static RobotCommand lastRecordedCmd = CMD_NONE;
static unsigned long lastRecordTime = 0;
static unsigned long playbackStartTime = 0;
static unsigned long currentPointStartTime = 0;

// Software timer for recording
static TimerHandle_t xPathTimer = NULL;

// Mutex for thread-safe access
static SemaphoreHandle_t xPathMutex = NULL;

// Timer callback for sampling commands
void pathTimerCallback(TimerHandle_t xTimer)
{
    // This is called periodically during recording
    // The actual recording happens in pathRecordCommand()
}

void pathMemoryInit()
{
    xPathMutex = xSemaphoreCreateMutex();

    // Create software timer for path sampling
    xPathTimer = xTimerCreate(
        "PathTimer",
        pdMS_TO_TICKS(PATH_RECORD_INTERVAL_MS),
        pdTRUE, // Auto-reload
        NULL,
        pathTimerCallback);

    pathClear();

    DEBUG_PRINTLN(F("Path memory initialized"));
}

void pathStartRecording()
{
    if (xPathMutex != NULL)
    {
        xSemaphoreTake(xPathMutex, portMAX_DELAY);
    }

    // Clear previous path
    pathPointCount = 0;
    lastRecordedCmd = CMD_NONE;
    lastRecordTime = millis();
    currentState = PATH_RECORDING;

    // Start timer
    if (xPathTimer != NULL)
    {
        xTimerStart(xPathTimer, 0);
    }

    if (xPathMutex != NULL)
    {
        xSemaphoreGive(xPathMutex);
    }

    DEBUG_PRINTLN(F("Path recording started"));
}

void pathStopRecording()
{
    if (xPathMutex != NULL)
    {
        xSemaphoreTake(xPathMutex, portMAX_DELAY);
    }

    // Record final duration for last command
    if (pathPointCount > 0 && lastRecordedCmd != CMD_NONE)
    {
        pathPoints[pathPointCount - 1].duration_ms = millis() - lastRecordTime;
    }

    currentState = PATH_IDLE;

    // Stop timer
    if (xPathTimer != NULL)
    {
        xTimerStop(xPathTimer, 0);
    }

    if (xPathMutex != NULL)
    {
        xSemaphoreGive(xPathMutex);
    }

    DEBUG_PRINTF("Path recording stopped. %d points recorded.\n", pathPointCount);
}

void pathStartPlayback()
{
    if (pathPointCount == 0)
    {
        DEBUG_PRINTLN(F("No path to play!"));
        return;
    }

    if (xPathMutex != NULL)
    {
        xSemaphoreTake(xPathMutex, portMAX_DELAY);
    }

    playbackIndex = 0;
    playbackStartTime = millis();
    currentPointStartTime = millis();
    currentState = PATH_PLAYING;

    if (xPathMutex != NULL)
    {
        xSemaphoreGive(xPathMutex);
    }

    DEBUG_PRINTF("Path playback started. %d points to play.\n", pathPointCount);
}

void pathStopPlayback()
{
    if (xPathMutex != NULL)
    {
        xSemaphoreTake(xPathMutex, portMAX_DELAY);
    }

    currentState = PATH_IDLE;
    playbackIndex = 0;

    if (xPathMutex != NULL)
    {
        xSemaphoreGive(xPathMutex);
    }

    // Send stop command
    if (xCommandQueue != NULL)
    {
        CommandMessage msg;
        msg.command = CMD_STOP;
        msg.speed = 0;
        msg.timestamp = millis();
        xQueueSend(xCommandQueue, &msg, 0);
    }

    DEBUG_PRINTLN(F("Path playback stopped"));
}

void pathClear()
{
    if (xPathMutex != NULL)
    {
        xSemaphoreTake(xPathMutex, portMAX_DELAY);
    }

    pathPointCount = 0;
    playbackIndex = 0;
    currentState = PATH_IDLE;
    lastRecordedCmd = CMD_NONE;

    if (xPathMutex != NULL)
    {
        xSemaphoreGive(xPathMutex);
    }

    DEBUG_PRINTLN(F("Path cleared"));
}

PathState pathGetState()
{
    return currentState;
}

int pathGetPointCount()
{
    return pathPointCount;
}

void pathRecordCommand(RobotCommand cmd)
{
    if (currentState != PATH_RECORDING)
    {
        return;
    }

    if (xPathMutex != NULL)
    {
        if (xSemaphoreTake(xPathMutex, pdMS_TO_TICKS(10)) != pdTRUE)
        {
            return;
        }
    }

    unsigned long now = millis();

    // If same command, just extend duration
    if (cmd == lastRecordedCmd && pathPointCount > 0)
    {
        // Duration will be calculated when command changes or recording stops
        if (xPathMutex != NULL)
            xSemaphoreGive(xPathMutex);
        return;
    }

    // Different command - save previous duration and start new point
    if (pathPointCount > 0 && lastRecordedCmd != CMD_NONE)
    {
        pathPoints[pathPointCount - 1].duration_ms = now - lastRecordTime;
    }

    // Add new point
    if (pathPointCount < MAX_PATH_POINTS)
    {
        pathPoints[pathPointCount].command = cmd;
        pathPoints[pathPointCount].duration_ms = 0; // Will be set on next change
        pathPointCount++;

        lastRecordedCmd = cmd;
        lastRecordTime = now;
    }
    else
    {
        DEBUG_PRINTLN(F("Path memory full!"));
    }

    if (xPathMutex != NULL)
    {
        xSemaphoreGive(xPathMutex);
    }
}

bool pathGetNextCommand(RobotCommand *cmd, uint32_t *duration_ms)
{
    if (currentState != PATH_PLAYING || playbackIndex >= pathPointCount)
    {
        currentState = PATH_IDLE;
        return false;
    }

    unsigned long now = millis();
    unsigned long elapsed = now - currentPointStartTime;

    // Check if current point duration has elapsed
    if (elapsed >= pathPoints[playbackIndex].duration_ms)
    {
        playbackIndex++;
        currentPointStartTime = now;

        if (playbackIndex >= pathPointCount)
        {
            currentState = PATH_IDLE;
            DEBUG_PRINTLN(F("Path playback complete"));
            return false;
        }
    }

    *cmd = pathPoints[playbackIndex].command;
    *duration_ms = pathPoints[playbackIndex].duration_ms;

    return true;
}

String pathGetJSON()
{
    // Create JSON array of path points
    DynamicJsonDocument doc(8192);
    JsonArray array = doc.to<JsonArray>();

    for (int i = 0; i < pathPointCount; i++)
    {
        JsonObject point = array.createNestedObject();

        switch (pathPoints[i].command)
        {
        case CMD_FORWARD:
            point["cmd"] = "forward";
            break;
        case CMD_BACKWARD:
            point["cmd"] = "backward";
            break;
        case CMD_LEFT:
            point["cmd"] = "left";
            break;
        case CMD_RIGHT:
            point["cmd"] = "right";
            break;
        case CMD_STOP:
            point["cmd"] = "stop";
            break;
        default:
            point["cmd"] = "none";
            break;
        }

        point["duration"] = pathPoints[i].duration_ms;
    }

    String result;
    serializeJson(doc, result);
    return result;
}

bool pathLoadFromJSON(const String &json)
{
    DynamicJsonDocument doc(8192);
    DeserializationError error = deserializeJson(doc, json);

    if (error)
    {
        DEBUG_PRINTF("JSON parse error: %s\n", error.c_str());
        return false;
    }

    if (!doc.is<JsonArray>())
    {
        DEBUG_PRINTLN(F("Expected JSON array"));
        return false;
    }

    if (xPathMutex != NULL)
    {
        xSemaphoreTake(xPathMutex, portMAX_DELAY);
    }

    pathPointCount = 0;
    JsonArray array = doc.as<JsonArray>();

    for (JsonObject point : array)
    {
        if (pathPointCount >= MAX_PATH_POINTS)
            break;

        String cmdStr = point["cmd"] | "none";
        uint32_t duration = point["duration"] | 0;

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

        pathPoints[pathPointCount].command = cmd;
        pathPoints[pathPointCount].duration_ms = duration;
        pathPointCount++;
    }

    if (xPathMutex != NULL)
    {
        xSemaphoreGive(xPathMutex);
    }

    DEBUG_PRINTF("Loaded %d path points from JSON\n", pathPointCount);
    return true;
}
