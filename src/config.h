#ifndef APP_CONFIG_H
#define APP_CONFIG_H

enum AppModeEnum { Auto, Manual };

extern AppModeEnum AppMode;
extern const bool WEATHER_FORECAST_ENABLED;

extern const int GMT_OFFSET_SEC;
extern const int DAYLIGHT_OFFSET_SEC;
extern const char* NTP_SERVER_URL;

extern const char* BACKEND_APP_URL;

extern const int DELAY_DIFF_BETWEEN_SERVOS_MILISECONDS;

extern const int MOVE_SMOOTHLY_MILISECONDS_INTERVAL;

#endif