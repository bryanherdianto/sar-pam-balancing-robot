#include "ble_control.h"
#include "mode_manager.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Command queue (created in main, shared here)
QueueHandle_t xCommandQueue = NULL;

// BLE objects
static BLEServer *pServer = NULL;
static BLECharacteristic *pCharacteristic = NULL;
static bool deviceConnected = false;
static bool bleInitialized = false;
static RobotCommand lastCommand = CMD_NONE;

// Joystick center deadzone
#define JOYSTICK_CENTER 127
#define JOYSTICK_DEADZONE 30

// BLE Server callbacks
class ServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer)
    {
        deviceConnected = true;
        DEBUG_PRINTLN(F("BLE device connected!"));
    }

    void onDisconnect(BLEServer *pServer)
    {
        deviceConnected = false;
        DEBUG_PRINTLN(F("BLE device disconnected"));

        // Restart advertising
        if (modeIsBLEActive())
        {
            pServer->getAdvertising()->start();
            DEBUG_PRINTLN(F("BLE advertising restarted"));
        }
    }
};

// Characteristic callbacks (receive data)
class CharacteristicCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        std::string value = pCharacteristic->getValue();

        if (value.length() > 0)
        {
            DEBUG_PRINT(F("BLE received: "));
            for (int i = 0; i < value.length(); i++)
            {
                DEBUG_PRINTF("%02X ", value[i]);
            }
            DEBUG_PRINTLN();

            // Parse the data
            RobotCommand cmd = bleParseJoystickData((uint8_t *)value.data(), value.length());

            if (cmd != CMD_NONE)
            {
                lastCommand = cmd;

                // Send to command queue
                if (xCommandQueue != NULL)
                {
                    CommandMessage msg;
                    msg.command = cmd;
                    msg.speed = 200; // Default speed
                    msg.timestamp = millis();

                    xQueueSend(xCommandQueue, &msg, 0); // Non-blocking
                }
            }
        }
    }
};

void bleInit()
{
    if (bleInitialized)
    {
        return;
    }

    DEBUG_PRINTLN(F("Initializing BLE..."));

    // Create command queue if not exists
    if (xCommandQueue == NULL)
    {
        xCommandQueue = xQueueCreate(COMMAND_QUEUE_SIZE, sizeof(CommandMessage));
    }

    // Initialize BLE
    BLEDevice::init(BLE_DEVICE_NAME);

    // Create BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    // Create BLE Service (HM-10 compatible UUID)
    BLEService *pService = pServer->createService(BLE_SERVICE_UUID);

    // Create BLE Characteristic
    pCharacteristic = pService->createCharacteristic(
        BLE_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_NOTIFY |
            BLECharacteristic::PROPERTY_WRITE_NR);

    pCharacteristic->setCallbacks(new CharacteristicCallbacks());
    pCharacteristic->addDescriptor(new BLE2902());

    // Start the service
    pService->start();

    bleInitialized = true;
    DEBUG_PRINTLN(F("BLE initialized"));
}

void bleStart()
{
    if (!bleInitialized)
    {
        bleInit();
    }

    // Start advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(BLE_SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    DEBUG_PRINTLN(F("BLE advertising started"));
    DEBUG_PRINTF("Device name: %s\n", BLE_DEVICE_NAME);
}

void bleStop()
{
    if (!bleInitialized)
    {
        return;
    }

    BLEDevice::stopAdvertising();
    DEBUG_PRINTLN(F("BLE advertising stopped"));
}

bool bleIsConnected()
{
    return deviceConnected;
}

RobotCommand bleGetLastCommand()
{
    return lastCommand;
}

RobotCommand bleParseJoystickData(uint8_t *data, size_t length)
{
    if (length == 0)
    {
        return CMD_NONE;
    }

    // Method 1: Single character commands (common format)
    if (length == 1)
    {
        switch (data[0])
        {
        case 'F':
        case 'f':
        case '1':
            return CMD_FORWARD;
        case 'B':
        case 'b':
        case '2':
            return CMD_BACKWARD;
        case 'L':
        case 'l':
        case '3':
            return CMD_LEFT;
        case 'R':
        case 'r':
        case '4':
            return CMD_RIGHT;
        case 'S':
        case 's':
        case '0':
            return CMD_STOP;
        }
    }

    // Method 2: Two-byte joystick data (X, Y each 0-255)
    // Many joystick apps send 2 bytes: X position, Y position
    if (length == 2)
    {
        int8_t x = data[0] - JOYSTICK_CENTER; // -127 to +128
        int8_t y = data[1] - JOYSTICK_CENTER;

        // Apply deadzone
        if (abs(x) < JOYSTICK_DEADZONE)
            x = 0;
        if (abs(y) < JOYSTICK_DEADZONE)
            y = 0;

        // Determine direction based on dominant axis
        if (abs(y) > abs(x))
        {
            // Vertical movement dominant
            if (y > JOYSTICK_DEADZONE)
                return CMD_FORWARD;
            if (y < -JOYSTICK_DEADZONE)
                return CMD_BACKWARD;
        }
        else
        {
            // Horizontal movement dominant
            if (x > JOYSTICK_DEADZONE)
                return CMD_RIGHT;
            if (x < -JOYSTICK_DEADZONE)
                return CMD_LEFT;
        }

        return CMD_STOP;
    }

    // Method 3: String format "X:nnn,Y:nnn" or similar
    // Parse as string
    String dataStr = String((char *)data);
    dataStr.trim();

    // Check for directional strings
    if (dataStr.equalsIgnoreCase("forward") || dataStr.equalsIgnoreCase("up"))
    {
        return CMD_FORWARD;
    }
    if (dataStr.equalsIgnoreCase("backward") || dataStr.equalsIgnoreCase("down"))
    {
        return CMD_BACKWARD;
    }
    if (dataStr.equalsIgnoreCase("left"))
    {
        return CMD_LEFT;
    }
    if (dataStr.equalsIgnoreCase("right"))
    {
        return CMD_RIGHT;
    }
    if (dataStr.equalsIgnoreCase("stop"))
    {
        return CMD_STOP;
    }

    // Method 4: Button number format (some apps send "B1", "B2", etc.)
    if (dataStr.startsWith("B") || dataStr.startsWith("b"))
    {
        int buttonNum = dataStr.substring(1).toInt();
        switch (buttonNum)
        {
        case 1:
            return CMD_FORWARD;
        case 2:
            return CMD_BACKWARD;
        case 3:
            return CMD_LEFT;
        case 4:
            return CMD_RIGHT;
        default:
            return CMD_STOP;
        }
    }

    DEBUG_PRINTLN(F("Unknown BLE data format"));
    return CMD_NONE;
}
