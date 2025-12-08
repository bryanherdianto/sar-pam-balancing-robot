#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// OPERATION MODES
typedef enum
{
    MODE_BALANCE = 1,      // Auto balance only
    MODE_BLE_CONTROL = 2,  // BLE Joystick control
    MODE_WIFI_CONTROL = 3, // WiFi/Flask control
    MODE_PATH_MEMORY = 4   // Path record/playback
} OperationMode;

// MOTOR PINS (L298N Driver)
// Motor A (Left)
#define PIN_ENA 5  // PWM speed control
#define PIN_IN1 16 // Direction
#define PIN_IN2 17 // Direction

// Motor B (Right)
#define PIN_ENB 18 // PWM speed control
#define PIN_IN3 32 // Direction
#define PIN_IN4 33 // Direction

// MPU6050 PINS
#define PIN_SDA 21
#define PIN_SCL 22
#define PIN_MPU_INT 19

// PWM SETTINGS
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8
#define PWM_CHANNEL_A 0
#define PWM_CHANNEL_B 1

// I2C SETTINGS
#define I2C_CLOCK_SPEED 400000

// PID PARAMETERS
#define PID_SETPOINT 190.0
#define PID_KP 25.0
#define PID_KI 80.0
#define PID_KD 1.2
#define PID_SAMPLE_TIME 10 // ms
#define PID_OUTPUT_MIN -255
#define PID_OUTPUT_MAX 255

// MOTOR SETTINGS
#define MIN_MOTOR_SPEED 50    // Minimum PWM to overcome friction
#define BALANCE_ANGLE_MIN 150 // Lower bound for balance range
#define BALANCE_ANGLE_MAX 220 // Upper bound for balance range

// WIFI SETTINGS
#define WIFI_SSID "YOUR_WIFI_SSID"         // Change to your WiFi name
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD" // Change to your WiFi password
#define WIFI_CONNECT_TIMEOUT 10000         // 10 seconds timeout
#define HTTP_SERVER_PORT 80

// BLE SETTINGS (HM-10 Compatible for BLE Joystick App)
#define BLE_DEVICE_NAME "ESP32-Robot"
// HM-10 UART Service UUID (used by BLE Joystick app)
#define BLE_SERVICE_UUID "0000FFE0-0000-1000-8000-00805F9B34FB"
#define BLE_CHARACTERISTIC_UUID "0000FFE1-0000-1000-8000-00805F9B34FB"

// COMMAND DEFINITIONS
typedef enum
{
    CMD_NONE = 0,
    CMD_FORWARD,
    CMD_BACKWARD,
    CMD_LEFT,
    CMD_RIGHT,
    CMD_STOP
} RobotCommand;

// Command structure for queue
typedef struct
{
    RobotCommand command;
    uint8_t speed; // 0-255
    uint32_t timestamp;
} CommandMessage;

// Path memory point structure
typedef struct
{
    RobotCommand command;
    uint32_t duration_ms; // How long to execute this command
} PathPoint;

// FREERTOS SETTINGS
// Task stack sizes
#define STACK_SIZE_BLE 4096
#define STACK_SIZE_WIFI 4096
#define STACK_SIZE_MODE 2048
#define STACK_SIZE_PATH 4096

// Task priorities (higher = more priority, max 24 on ESP32)
#define PRIORITY_BLE 2
#define PRIORITY_WIFI 2
#define PRIORITY_MODE 3
#define PRIORITY_PATH 2

// Queue sizes
#define COMMAND_QUEUE_SIZE 10
#define PATH_QUEUE_SIZE 100

// Timer periods
#define TELEMETRY_PERIOD_MS 500
#define STATUS_LED_PERIOD_MS 1000
#define PATH_SAMPLE_PERIOD_MS 100

// PATH MEMORY SETTINGS
#define MAX_PATH_POINTS 500
#define PATH_RECORD_INTERVAL_MS 100

// DEBUG SETTINGS
#define DEBUG_SERIAL_BAUD 115200
#define DEBUG_ENABLED true

#if DEBUG_ENABLED
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTF(...)
#endif

#endif // CONFIG_H
