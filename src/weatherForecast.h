#ifndef WEATHER_FORECAST_H
#define WEATHER_FORECAST_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <vector>

using namespace std;

const int BUFFER_SIZE = 1024;

struct WeatherItem {
    float temperature;
    float windSpeed;
    float hoursAhead;
    String date;
};

class WeatherForecast {

    private:
        HTTPClient& httpClient;
        const char* weatherApiUrl;
        const char* weatherApiKey;
        double locationLat;
        double locationLon;

        
        String buildUrl();

    public:
        WeatherForecast(
            HTTPClient& httpClient,
            const char* weatherApiUrl,
            const char* weatherApiKey,
            double locationLat,
            double locationLon
        );

        vector<WeatherItem> fetchData();

};

#endif