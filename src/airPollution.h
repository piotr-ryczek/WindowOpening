#ifndef AIR_POLLUTION_H
#define AIR_POLLUTION_H

#include <Arduino.h>
#include <HTTPClient.h>

struct AirPollutionItem {
    float pm25;
    String pm25Date;
    float pm10;
    String pm10Date;
};

struct SensorItem {
    float pollutionValue;
    String date;
};

class AirPollution {

    private:
        HTTPClient& httpClient;
        const char* pollutionApiUrl;
        int sensorPM25Id;
        int sensorPM10Id;

        String buildPM25Url();
        String buildPM10Url();
        SensorItem fetchPMData(String url);

    public:
        AirPollution(
            HTTPClient& httpClient,
            const char* pollutionApiUrl, 
            int sensorPM25Id,
            int sensorPM10Id
        );

        AirPollutionItem fetchData();
};

#endif