#include <config.h>

AppModeEnum AppMode = Manual; // Always initializing with Manual for security reasons (not stored in memory)
const bool WEATHER_FORECAST_ENABLED = true;

const int GMT_OFFSET_SEC = 3600;
const int DAYLIGHT_OFFSET_SEC = 3600;
const char* NTP_SERVER_URL = "pool.ntp.org";

const char* BACKEND_APP_URL = "http://window-opening.nero12.usermd.net";

const int DELAY_DIFF_BETWEEN_SERVOS_MILISECONDS = 500; // 0.5s - difference between one will start before other one

const int MOVE_SMOOTHLY_MILISECONDS_INTERVAL = 40;