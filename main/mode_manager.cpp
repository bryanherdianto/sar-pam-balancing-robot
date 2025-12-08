#include "mode_manager.h"
#include "power_manager.h"

// Current mode
static OperationMode currentMode = MODE_BALANCE;
static SemaphoreHandle_t xModeMutex = NULL;

// Forward declarations
void modeManagerTask(void *pvParameters);

// Mode names
static const char *modeNames[] = {
    "Unknown",
    "Auto Balance",
    "BLE Control",
    "WiFi Control",
    "Path Memory"};

void modeManagerInit()
{
    xModeMutex = xSemaphoreCreateMutex();

    // Create mode manager task on Core 1
    xTaskCreatePinnedToCore(
        modeManagerTask,
        "ModeManager",
        STACK_SIZE_MODE,
        NULL,
        PRIORITY_MODE,
        NULL,
        1 // Core 1
    );

    DEBUG_PRINTLN(F("Mode manager initialized"));
    modePrintMenu();
}

void modeManagerTask(void *pvParameters)
{
    char inputBuffer[10];
    int bufferIndex = 0;

    while (true)
    {
        // Check for serial input
        while (Serial.available() > 0)
        {
            char c = Serial.read();

            if (c == '\n' || c == '\r')
            {
                if (bufferIndex > 0)
                {
                    inputBuffer[bufferIndex] = '\0';
                    int modeNum = atoi(inputBuffer);

                    if (modeNum >= 1 && modeNum <= 4)
                    {
                        modeSet((OperationMode)modeNum);
                    }
                    else
                    {
                        DEBUG_PRINTLN(F("Invalid mode. Enter 1-4."));
                        modePrintMenu();
                    }
                    bufferIndex = 0;
                }
            }
            else if (bufferIndex < 9 && c >= '0' && c <= '9')
            {
                inputBuffer[bufferIndex++] = c;
            }
        }

        // Yield to other tasks
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

OperationMode modeGetCurrent()
{
    OperationMode mode;

    if (xModeMutex != NULL)
    {
        xSemaphoreTake(xModeMutex, portMAX_DELAY);
    }
    mode = currentMode;
    if (xModeMutex != NULL)
    {
        xSemaphoreGive(xModeMutex);
    }

    return mode;
}

void modeSet(OperationMode newMode)
{
    if (xModeMutex != NULL)
    {
        xSemaphoreTake(xModeMutex, portMAX_DELAY);
    }

    OperationMode oldMode = currentMode;
    currentMode = newMode;

    if (xModeMutex != NULL)
    {
        xSemaphoreGive(xModeMutex);
    }

    DEBUG_PRINTF("\n=== Mode changed: %s -> %s ===\n",
                 modeGetName(oldMode), modeGetName(newMode));

    // Handle power management based on mode
    switch (newMode)
    {
    case MODE_BALANCE:
        // Balance only - turn off wireless
        powerDisableBLE();
        powerDisableWiFi();
        break;

    case MODE_BLE_CONTROL:
        // BLE control - enable BLE, disable WiFi
        powerDisableWiFi();
        powerEnableBLE();
        break;

    case MODE_WIFI_CONTROL:
        // WiFi control - enable WiFi, disable BLE
        powerDisableBLE();
        powerEnableWiFi();
        break;

    case MODE_PATH_MEMORY:
        // Path memory - enable WiFi for Flask interface, disable BLE
        powerDisableBLE();
        powerEnableWiFi();
        break;
    }

    modePrintMenu();
}

const char *modeGetName(OperationMode mode)
{
    if (mode >= MODE_BALANCE && mode <= MODE_PATH_MEMORY)
    {
        return modeNames[mode];
    }
    return modeNames[0];
}

void modePrintMenu()
{
    DEBUG_PRINTLN(F("\n========== ROBOT CONTROL MENU =========="));
    DEBUG_PRINTLN(F("Enter mode number to switch:"));
    DEBUG_PRINTLN(F("  1 - Auto Balance (standalone)"));
    DEBUG_PRINTLN(F("  2 - BLE Control (BLE Joystick app)"));
    DEBUG_PRINTLN(F("  3 - WiFi Control (Flask web interface)"));
    DEBUG_PRINTLN(F("  4 - Path Memory (record/playback)"));
    DEBUG_PRINTF("Current mode: [%d] %s\n", currentMode, modeGetName(currentMode));
    DEBUG_PRINTLN(F("=========================================\n"));
}

bool modeIsBalancing()
{
    // Always balancing in all modes
    return true;
}

bool modeIsBLEActive()
{
    return modeGetCurrent() == MODE_BLE_CONTROL;
}

bool modeIsWiFiActive()
{
    OperationMode mode = modeGetCurrent();
    return (mode == MODE_WIFI_CONTROL || mode == MODE_PATH_MEMORY);
}

bool modeIsPathActive()
{
    return modeGetCurrent() == MODE_PATH_MEMORY;
}
