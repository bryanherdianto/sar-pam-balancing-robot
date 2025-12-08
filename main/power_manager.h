#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include "config.h"

/**
 * Initialize power manager
 */
void powerManagerInit();

/**
 * Enable WiFi (connect to router)
 */
void powerEnableWiFi();

/**
 * Disable WiFi to save power
 */
void powerDisableWiFi();

/**
 * Enable BLE
 */
void powerEnableBLE();

/**
 * Disable BLE to save power
 */
void powerDisableBLE();

/**
 * Check if WiFi is currently enabled
 */
bool powerIsWiFiEnabled();

/**
 * Check if BLE is currently enabled
 */
bool powerIsBLEEnabled();

/**
 * Get WiFi connection status
 */
bool powerIsWiFiConnected();

/**
 * Get ESP32 IP address (when WiFi connected)
 */
String powerGetIPAddress();

#endif // POWER_MANAGER_H
