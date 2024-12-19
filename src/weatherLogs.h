#ifndef WEATHER_LOGS_H
#define WEATHER_LOGS_H

#include <vector>
#include <Arduino.h>

using namespace std;

const int MAX_WEATHER_LOGS = 10;

struct WeatherLog {
    double outsideTemperature;
    double windSpeed;
    double pm25;
    double pm10;
    String forecastDate;
    String pm25Date;
    String pm10Date;
};

extern vector<WeatherLog> weatherLogs;

void addWeatherLog(double outsideTemperature, double windSpeed, String forecastDate, double pm25, String pm25Date, double pm10, String pm10Date);
WeatherLog* getLastWeatherLogNotTooOld(double maxHoursOld);

#endif