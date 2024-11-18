#ifndef AIR_POLLUTION_H
#define AIR_POLLUTION_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <backgroundApp.h>

struct AirPollutionItem {
    double pm25; // Max norm: 15
    String pm25Date;
    double pm10; // Max norm: 45
    String pm10Date;
};

struct SensorItem {
    double pollutionValue;
    String date;
};

class AirPollution {

    private:
        HTTPClient* httpClient;
        BackgroundApp* backgroundApp;
        const char* pollutionApiUrl;
        int sensorPM25Id;
        int sensorPM10Id;

        String buildPM25Url();
        String buildPM10Url();
        SensorItem fetchPMData(String url);

    public:
        AirPollution(
            HTTPClient* httpClient,
            BackgroundApp* backgroundApp,
            const char* pollutionApiUrl, 
            int sensorPM25Id,
            int sensorPM10Id
        );

        AirPollutionItem fetchData();
};

#endif