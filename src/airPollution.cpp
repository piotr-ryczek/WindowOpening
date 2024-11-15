#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <airPollution.h>

AirPollution::AirPollution(
            HTTPClient& httpClient,
            const char* pollutionApiUrl, 
            int sensorPM25Id,
            int sensorPM10Id):
            httpClient(httpClient),
            pollutionApiUrl(pollutionApiUrl),
            sensorPM25Id(sensorPM25Id),
            sensorPM10Id(sensorPM10Id) {}

String AirPollution::buildPM25Url() {
    return String(pollutionApiUrl) + String(sensorPM25Id);
}

String AirPollution::buildPM10Url() {
    return String(pollutionApiUrl) + String(sensorPM10Id);
}

SensorItem AirPollution::fetchPMData(String url) {
    httpClient.begin(url);

    int httpCode = httpClient.GET();

    if (httpCode == 0) {
        Serial.println("PM Request failed");
        return SensorItem{};
    }

    if (httpCode != 200) {
        Serial.println("PM Request didn't respond with 200");
        return SensorItem{};
    }

    String payload = httpClient.getString();

    JsonDocument doc;
    deserializeJson(doc, payload);

    auto firstResult = doc["values"][0];

    return SensorItem{
        pollutionValue: firstResult["value"],
        date: firstResult["date"]
    };
}

AirPollutionItem AirPollution::fetchData() {
    String pm25Url = buildPM25Url();
    String pm10Url = buildPM10Url();

    SensorItem pm25Data = fetchPMData(pm25Url);
    SensorItem pm10Data = fetchPMData(pm10Url);

    return AirPollutionItem{
        pm25: pm25Data.pollutionValue,
        pm25Date: pm25Data.date,
        pm10: pm10Data.pollutionValue,
        pm10Date: pm25Data.date
    };
}
