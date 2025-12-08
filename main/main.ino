/**
 * @file self_balancing_robot.ino
 * @brief Main file for ESP32 Self-Balancing Robot with Multi-Mode Control
 *
 * Architecture: Hybrid Bare-Metal + FreeRTOS
 * - Balance loop runs in loop() on Core 0 (bare-metal, no task switching latency)
 * - Communication tasks (BLE, WiFi, Mode Manager) run on Core 1 (FreeRTOS)
 *
 * Modes:
 * 1. Auto Balance - Standalone balancing
 * 2. BLE Control  - Control via BLE Joystick app
 * 3. WiFi Control - Control via Flask web interface
 * 4. Path Memory  - Record and playback movement paths
 *
 * Switch modes by entering 1-4 in Serial Monitor.
 */

#include "config.h"
#include "motor_control.h"
#include "mpu6050_handler.h"
#include "pid_controller.h"
#include "mode_manager.h"
#include "power_manager.h"
#include "ble_control.h"
#include "wifi_control.h"
#include "path_memory.h"

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

// Current steering command from BLE/WiFi
static RobotCommand currentCommand = CMD_NONE;
static unsigned long lastCommandTime = 0;
#define COMMAND_TIMEOUT_MS 500 // Commands expire after 500ms

// Telemetry data
static double currentAngle = 0;
static double currentOutput = 0;

static TaskHandle_t xBLETaskHandle = NULL;
static TaskHandle_t xWiFiTaskHandle = NULL;

/**
 * BLE Control Task
 * Handles BLE advertising and connection when in BLE mode
 */
void bleTasks(void *pvParameters)
{
    DEBUG_PRINTLN(F("BLE task started on Core 1"));

    while (true)
    {
        if (modeIsBLEActive())
        {
            // BLE is managed by callbacks, just yield
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        else
        {
            // Not in BLE mode, wait longer
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}

/**
 * WiFi Control Task
 * Handles HTTP server and Flask communication
 */
void wifiTask(void *pvParameters)
{
    DEBUG_PRINTLN(F("WiFi task started on Core 1"));

    bool serverInitialized = false;

    while (true)
    {
        if (modeIsWiFiActive())
        {
            // Initialize server if WiFi just connected
            if (powerIsWiFiConnected() && !serverInitialized)
            {
                wifiServerInit();
                wifiServerStart();
                serverInitialized = true;
            }

            // Handle HTTP requests
            if (serverInitialized)
            {
                wifiServerHandle();
            }

            vTaskDelay(pdMS_TO_TICKS(10)); // Responsive HTTP handling
        }
        else
        {
            if (serverInitialized)
            {
                wifiServerStop();
                serverInitialized = false;
            }
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}

/**
 * Path Playback Task
 * Handles path playback when in path memory mode
 */
void pathPlaybackTask(void *pvParameters)
{
    DEBUG_PRINTLN(F("Path playback task started on Core 1"));

    while (true)
    {
        if (pathGetState() == PATH_PLAYING)
        {
            RobotCommand cmd;
            uint32_t duration;

            if (pathGetNextCommand(&cmd, &duration))
            {
                // Send command to main loop
                if (xCommandQueue != NULL)
                {
                    CommandMessage msg;
                    msg.command = cmd;
                    msg.speed = 200;
                    msg.timestamp = millis();
                    xQueueSend(xCommandQueue, &msg, 0);
                }
            }
            vTaskDelay(pdMS_TO_TICKS(PATH_RECORD_INTERVAL_MS));
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

static TimerHandle_t xTelemetryTimer = NULL;
static TimerHandle_t xStatusTimer = NULL;

void telemetryTimerCallback(TimerHandle_t xTimer)
{
    // Send telemetry to WiFi if active
    if (modeIsWiFiActive())
    {
        wifiSendTelemetry(currentAngle, currentOutput);
    }
}

void statusTimerCallback(TimerHandle_t xTimer)
{
    // Periodic status output
    static int statusCounter = 0;
    if (++statusCounter >= 10)
    { // Every 10 seconds
        statusCounter = 0;
        DEBUG_PRINTF("[Status] Mode: %s, Angle: %.1f, Output: %.1f\n",
                     modeGetName(modeGetCurrent()), currentAngle, currentOutput);

        if (modeIsWiFiActive() && powerIsWiFiConnected())
        {
            DEBUG_PRINTF("[WiFi] IP: %s\n", powerGetIPAddress().c_str());
        }
        if (modeIsBLEActive())
        {
            DEBUG_PRINTF("[BLE] Connected: %s\n", bleIsConnected() ? "Yes" : "No");
        }
    }
}

void setup()
{
    Serial.begin(DEBUG_SERIAL_BAUD);
    delay(1000); // Wait for serial

    DEBUG_PRINTLN(F("\n========================================"));
    DEBUG_PRINTLN(F("  ESP32 Self-Balancing Robot v2.0"));
    DEBUG_PRINTLN(F("  Hybrid Architecture: Balance + FreeRTOS"));
    DEBUG_PRINTLN(F("========================================\n"));

    // Initialize modules
    DEBUG_PRINTLN(F("[Init] Power manager..."));
    powerManagerInit();

    DEBUG_PRINTLN(F("[Init] Motor control..."));
    motorInit();

    DEBUG_PRINTLN(F("[Init] MPU6050..."));
    if (!mpuInit())
    {
        DEBUG_PRINTLN(F("MPU6050 init failed! Halting."));
        while (1)
        {
            delay(1000);
        }
    }

    DEBUG_PRINTLN(F("[Init] PID controller..."));
    pidInit();

    DEBUG_PRINTLN(F("[Init] Path memory..."));
    pathMemoryInit();

    DEBUG_PRINTLN(F("[Init] BLE..."));
    bleInit();

    // Create command queue
    if (xCommandQueue == NULL)
    {
        xCommandQueue = xQueueCreate(COMMAND_QUEUE_SIZE, sizeof(CommandMessage));
    }

    // Create FreeRTOS tasks on Core 1 (Core 0 is for balance loop)
    DEBUG_PRINTLN(F("[Init] Creating FreeRTOS tasks..."));

    xTaskCreatePinnedToCore(
        bleTasks,
        "BLETask",
        STACK_SIZE_BLE,
        NULL,
        PRIORITY_BLE,
        &xBLETaskHandle,
        1 // Core 1
    );

    xTaskCreatePinnedToCore(
        wifiTask,
        "WiFiTask",
        STACK_SIZE_WIFI,
        NULL,
        PRIORITY_WIFI,
        &xWiFiTaskHandle,
        1 // Core 1
    );

    xTaskCreatePinnedToCore(
        pathPlaybackTask,
        "PathTask",
        STACK_SIZE_PATH,
        NULL,
        PRIORITY_PATH,
        NULL,
        1 // Core 1
    );

    // Create software timers
    DEBUG_PRINTLN(F("[Init] Creating software timers..."));

    xTelemetryTimer = xTimerCreate(
        "Telemetry",
        pdMS_TO_TICKS(TELEMETRY_PERIOD_MS),
        pdTRUE, // Auto-reload
        NULL,
        telemetryTimerCallback);
    xTimerStart(xTelemetryTimer, 0);

    xStatusTimer = xTimerCreate(
        "Status",
        pdMS_TO_TICKS(STATUS_LED_PERIOD_MS),
        pdTRUE,
        NULL,
        statusTimerCallback);
    xTimerStart(xStatusTimer, 0);

    // Initialize mode manager (starts its own task)
    DEBUG_PRINTLN(F("[Init] Mode manager..."));
    modeManagerInit();

    DEBUG_PRINTLN(F("\n[Init] Complete! Robot ready."));
    DEBUG_PRINTLN(F("Balance loop running on Core 0 (bare-metal)"));
    DEBUG_PRINTLN(F("Communication tasks running on Core 1 (FreeRTOS)\n"));
}

void loop()
{
    // This runs on Core 0 with maximum priority
    // No FreeRTOS task switching overhead for critical balance control

    if (!mpuIsReady())
    {
        return;
    }

    // Check for steering commands from queue (non-blocking)
    if (xCommandQueue != NULL)
    {
        CommandMessage msg;
        if (xQueueReceive(xCommandQueue, &msg, 0) == pdTRUE)
        {
            currentCommand = msg.command;
            lastCommandTime = msg.timestamp;

            // Record command if in path recording mode
            if (pathGetState() == PATH_RECORDING)
            {
                pathRecordCommand(msg.command);
            }
        }
    }

    // Expire old commands
    if (millis() - lastCommandTime > COMMAND_TIMEOUT_MS)
    {
        if (currentCommand != CMD_NONE && currentCommand != CMD_STOP)
        {
            currentCommand = CMD_NONE;
        }
    }

    // Read sensor data
    mpuClearInterrupt();
    double angle;

    if (!mpuReadAngle(&angle))
    {
        // No valid data, skip this cycle
        return;
    }

    currentAngle = angle;

    // Compute PID
    double output;
    if (!pidCompute(angle, &output))
    {
        motorStop();
        return;
    }

    currentOutput = output;

    // Check if within balance range
    if (angle < BALANCE_ANGLE_MIN || angle > BALANCE_ANGLE_MAX)
    {
        motorStop();
        return;
    }

    // Apply motor control with steering
    if (output > 0)
    {
        // Tilting forward - need to move forward
        int baseSpeed = (int)output;

        switch (currentCommand)
        {
        case CMD_LEFT:
            // Turn left: reduce left motor
            motorSetSpeeds(baseSpeed / 2, baseSpeed);
            break;
        case CMD_RIGHT:
            // Turn right: reduce right motor
            motorSetSpeeds(baseSpeed, baseSpeed / 2);
            break;
        default:
            // Straight forward
            motorForward(baseSpeed);
            break;
        }
    }
    else if (output < 0)
    {
        // Tilting backward - need to move backward
        int baseSpeed = (int)(-output);

        switch (currentCommand)
        {
        case CMD_LEFT:
            motorSetSpeeds(-baseSpeed / 2, -baseSpeed);
            break;
        case CMD_RIGHT:
            motorSetSpeeds(-baseSpeed, -baseSpeed / 2);
            break;
        default:
            motorReverse(baseSpeed);
            break;
        }
    }
    else
    {
        motorStop();
    }

    // Small yield to prevent watchdog trigger (but not a full task delay)
    yield();
}