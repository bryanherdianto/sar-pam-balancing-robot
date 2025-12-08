#include "power_manager.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_bt.h>
#include <esp_bt_main.h>

// State tracking
static bool wifiEnabled = false;
static bool bleEnabled = false;
static bool wifiConnected = false;

void powerManagerInit()
{
    // Start with everything off
    WiFi.mode(WIFI_OFF);
    esp_wifi_stop();

    // BLE is managed by ble_control module
    // Just track state here

    DEBUG_PRINTLN(F("Power manager initialized (WiFi/BLE off)"));
}

void powerEnableWiFi()
{
    if (wifiEnabled)
    {
        DEBUG_PRINTLN(F("WiFi already enabled"));
        return;
    }

    DEBUG_PRINTLN(F("Enabling WiFi..."));

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    DEBUG_PRINT(F("Connecting to WiFi"));

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED)
    {
        if (millis() - startTime > WIFI_CONNECT_TIMEOUT)
        {
            DEBUG_PRINTLN(F("\nWiFi connection timeout!"));
            wifiEnabled = true; // Still mark as enabled (trying)
            wifiConnected = false;
            return;
        }
        delay(500);
        DEBUG_PRINT(".");
    }

    wifiEnabled = true;
    wifiConnected = true;

    DEBUG_PRINTLN(F("\nWiFi connected!"));
    DEBUG_PRINT(F("IP Address: "));
    DEBUG_PRINTLN(WiFi.localIP());
}

void powerDisableWiFi()
{
    if (!wifiEnabled)
    {
        DEBUG_PRINTLN(F("WiFi already disabled"));
        return;
    }

    DEBUG_PRINTLN(F("Disabling WiFi..."));

    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    esp_wifi_stop();

    wifiEnabled = false;
    wifiConnected = false;

    DEBUG_PRINTLN(F("WiFi disabled"));
}

void powerEnableBLE()
{
    if (bleEnabled)
    {
        DEBUG_PRINTLN(F("BLE already enabled"));
        return;
    }

    DEBUG_PRINTLN(F("Enabling BLE..."));

    // BLE initialization is handled by ble_control module
    // This just tracks the state and ensures BT controller is enabled
    if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE)
    {
        esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
        esp_bt_controller_init(&bt_cfg);
    }

    if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_INITED)
    {
        esp_bt_controller_enable(ESP_BT_MODE_BLE);
    }

    bleEnabled = true;
    DEBUG_PRINTLN(F("BLE enabled"));
}

void powerDisableBLE()
{
    if (!bleEnabled)
    {
        DEBUG_PRINTLN(F("BLE already disabled"));
        return;
    }

    DEBUG_PRINTLN(F("Disabling BLE..."));

    // Disable BT controller
    if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED)
    {
        esp_bt_controller_disable();
    }

    bleEnabled = false;
    DEBUG_PRINTLN(F("BLE disabled"));
}

bool powerIsWiFiEnabled()
{
    return wifiEnabled;
}

bool powerIsBLEEnabled()
{
    return bleEnabled;
}

bool powerIsWiFiConnected()
{
    if (!wifiEnabled)
        return false;
    wifiConnected = (WiFi.status() == WL_CONNECTED);
    return wifiConnected;
}

String powerGetIPAddress()
{
    if (wifiConnected)
    {
        return WiFi.localIP().toString();
    }
    return "Not connected";
}
