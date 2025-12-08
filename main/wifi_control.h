#ifndef WIFI_CONTROL_H
#define WIFI_CONTROL_H

#include "config.h"

/**
 * Initialize WiFi WebSocket server
 * Must be called after WiFi is connected
 */
void wifiServerInit();

/**
 * Start WebSocket server
 */
void wifiServerStart();

/**
 * Stop WebSocket server
 */
void wifiServerStop();

/**
 * Handle WebSocket events (call in task loop)
 */
void wifiServerHandle();

/**
 * Send telemetry data to all connected WebSocket clients
 */
void wifiSendTelemetry(double angle, double output);

/**
 * Broadcast a message to all connected clients
 */
void wifiBroadcast(const String &message);

/**
 * Check if any client is connected
 */
bool wifiHasClients();

#endif // WIFI_CONTROL_H
