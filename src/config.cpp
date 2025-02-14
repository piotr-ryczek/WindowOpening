#include <config.h>
#include <Arduino.h>

AppModeEnum AppMode = Manual; // Always initializing with Manual for security reasons (not stored in memory)
const bool WEATHER_FORECAST_ENABLED = true;
bool forceOpeningWindowCalculation = false; // Flag to recalculate manually WindowOpening any time
bool hasNTPAlreadyConfigured = false; // Can be changed only once
bool isNTPUnderConfiguration = false;
bool isWifiConnected = false;
bool isWifiConnecting = false;
bool shouldTryToConnectToWifi = false;
bool isBLEClientConnected = false;

const int GMT_OFFSET_SEC = 3600;
const int DAYLIGHT_OFFSET_SEC = 3600;
const char* NTP_SERVER_URL = "pool.ntp.org";

const char* BACKEND_APP_URL = "http://window-opening.nero12.usermd.net";

const int DELAY_DIFF_BETWEEN_SERVOS_MILISECONDS = 500; // 0.5s - difference between one will start before other one

const int MOVE_SMOOTHLY_MILISECONDS_INTERVAL = 40;

const uint32_t VOLTAGE_REFERENCE = 1100;
const float RESISTOR_FIRST_VALUE = 5000.0;  // 5kΩ
const float RESISTOR_SECOND_VALUE = 2150.0;   // 2.15kΩ

const float BATTERY_VOLTAGE_0_REFERENCE = 6.5;
const float BATTERY_VOLTAGE_100_REFERENCE = 8.45;
const float BATTERY_VOLTAGE_MIN_PERCENTAGE = 20;  