#ifndef BLE_CONTROL_H
#define BLE_CONTROL_H

#include "config.h"

// Command queue handle (shared with main loop)
extern QueueHandle_t xCommandQueue;

/**
 * Initialize BLE with HM-10 compatible service
 */
void bleInit();

/**
 * Start BLE advertising
 */
void bleStart();

/**
 * Stop BLE and release resources
 */
void bleStop();

/**
 * Check if a device is connected
 */
bool bleIsConnected();

/**
 * Get last received command
 */
RobotCommand bleGetLastCommand();

/**
 * Parse joystick data from BLE Joystick app
 */
RobotCommand bleParseJoystickData(uint8_t *data, size_t length);

#endif // BLE_CONTROL_H
