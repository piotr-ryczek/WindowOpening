#include <iostream>
#include <weatherLogs.h>
#include <timeHelpers.h>

vector<WeatherLog> weatherLogs;

using namespace std;

void addWeatherLog(double outsideTemperature, String outsideTemperatureDate, double pm25, String pm25Date, double pm10, String pm10Date) {
    WeatherLog newWeatherLog;

    newWeatherLog.outsideTemperature = outsideTemperature;
    newWeatherLog.outsideTemperatureDate = outsideTemperatureDate;
    newWeatherLog.pm25 = pm25;
    newWeatherLog.pm25Date = pm25Date;
    newWeatherLog.pm10 = pm10;
    newWeatherLog.pm10Date = pm10Date;


    weatherLogs.push_back(newWeatherLog);

    if (weatherLogs.size() > MAX_WEATHER_LOGS) {
        weatherLogs.erase(weatherLogs.begin());
    }
}

WeatherLog* getLastWeatherLogNotTooOld(double maxHoursOld) {
    if (weatherLogs.size() == 0) {
        return nullptr;
    }

    WeatherLog* lastWeatherLog = &weatherLogs.back();

    float outsideTemperatureHoursAhead = calculateHoursAhead(lastWeatherLog->outsideTemperatureDate);
    float pm25HoursAhead = calculateHoursAhead(lastWeatherLog->pm25Date);
    float pm10HoursAhead = calculateHoursAhead(lastWeatherLog->pm10Date);

    // Expecting results which aren't too outdated
    if (
        outsideTemperatureHoursAhead < -maxHoursOld ||
        pm25HoursAhead < -maxHoursOld ||
        pm10HoursAhead < -maxHoursOld
    ) {
        return nullptr;
    }
    
    return lastWeatherLog;
}