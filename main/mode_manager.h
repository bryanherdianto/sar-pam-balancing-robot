#ifndef MODE_MANAGER_H
#define MODE_MANAGER_H

#include "config.h"

/**
 * Initialize mode manager
 * Creates FreeRTOS task for serial input monitoring
 */
void modeManagerInit();

/**
 * Get current operation mode
 */
OperationMode modeGetCurrent();

/**
 * Set new operation mode
 * Handles power management (turning on/off WiFi/BLE)
 */
void modeSet(OperationMode newMode);

/**
 * Get mode name as string
 */
const char *modeGetName(OperationMode mode);

/**
 * Print mode menu to Serial
 */
void modePrintMenu();

/**
 * Check if balancing should be active
 * (true for all modes - we always balance)
 */
bool modeIsBalancing();

/**
 * Check if BLE should be active
 */
bool modeIsBLEActive();

/**
 * Check if WiFi should be active
 */
bool modeIsWiFiActive();

/**
 * Check if path memory is active
 */
bool modeIsPathActive();

#endif // MODE_MANAGER_H
