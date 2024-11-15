#include <config.h>

AppModeEnum AppMode = Manual; // Always initializing with Manual for security reasons (not stored in memory)
const bool WEATHER_FORECAST_ENABLED = true;

const int GMT_OFFSET_SEC = 3600;
const int DAYLIGHT_OFFSET_SEC = 3600;
const char* NTP_SERVER_URL = "pool.ntp.org";

const int AIR_POLLUTION_SENSOR_PM_25_ID = 2071;
const int AIR_POLLUTION_SENSOR_PM_10_ID = 2069;
const char* AIR_POLLUTION_SENSOR_API_URL = "https://api.gios.gov.pl/pjp-api/rest/data/getData/";
const char* WEATHER_FORECAST_API_URL = "https://api.openweathermap.org/data/2.5/";

const double LOCATION_LAT = 51.760229;
const double LOCATION_LON = 19.550675;

const int DELAY_DIFF_BETWEEN_SERVOS_MILISECONDS = 500; // 0.5s - difference between one will start before other one

const int MOVE_SMOOTHLY_MILISECONDS_INTERVAL = 40;