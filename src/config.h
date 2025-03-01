#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <Arduino.h>

enum AppModeEnum { Auto, Manual };

extern AppModeEnum AppMode;
extern const bool WEATHER_FORECAST_ENABLED;

// Flags
extern bool forceOpeningWindowCalculation;
extern bool hasNTPAlreadyConfigured;
extern bool isNTPUnderConfiguration;
extern bool isWifiConnected;
extern bool isWifiConnecting;
extern bool shouldTryToConnectToWifi;
extern bool isBLEClientConnected;

extern const int GMT_OFFSET_SEC;
extern const int DAYLIGHT_OFFSET_SEC;
extern const char* NTP_SERVER_URL;

extern const char* BACKEND_APP_URL;

extern const int DELAY_DIFF_BETWEEN_SERVOS_MILISECONDS;

extern const int MOVE_SMOOTHLY_MILISECONDS_INTERVAL;

extern const uint32_t VOLTAGE_REFERENCE;
extern const float RESISTOR_FIRST_VALUE;
extern const float RESISTOR_SECOND_VALUE;

extern const float BATTERY_VOLTAGE_0_REFERENCE;
extern const float BATTERY_VOLTAGE_100_REFERENCE;
extern const float BATTERY_VOLTAGE_MIN_PERCENTAGE;

extern float lastReadBatteryVoltageBox;
extern float lastReadBatteryVoltageServos;

#endif