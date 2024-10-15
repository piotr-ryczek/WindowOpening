#include <weatherForecast.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <stdexcept>
#include <string>
#include <vector>
#include <timeHelpers.h>

using namespace std;

WeatherForecast::WeatherForecast(
            HTTPClient& httpClient,
            const char* weatherApiUrl,
            const char* weatherApiKey,
            double locationLat,
            double locationLon):
            httpClient(httpClient),
            weatherApiUrl(weatherApiUrl), 
            weatherApiKey(weatherApiKey), 
            locationLat(locationLat), 
            locationLon(locationLon) {}

String WeatherForecast::buildUrl() {
    return String(weatherApiUrl) + "forecast?lat=" + locationLat + "&lon=" + locationLon + "&appid=" + weatherApiKey + "&units=Metric";
}

vector<WeatherItem> WeatherForecast::fetchData() {
    String url = buildUrl();
    vector<WeatherItem> parsedData;

    httpClient.begin(url);

    int httpCode = httpClient.GET();

    if (httpCode == 0) {
        throw std::runtime_error("WeatherForecast Request failed");
    }

    if (httpCode != 200) {
        Serial.println("WeatherForecast didn't respond with 200");
        return parsedData;
    }

    String payload = httpClient.getString();

    JsonDocument doc;
    deserializeJson(doc, payload);

    for (const auto& item: doc["list"].as<JsonArray>()) {
        float temperature = item["main"]["temp"];
        float windSpeed = item["wind"]["speed"];
        String date = item["dt_txt"];
        float hoursAhead = calculateHoursAhead(date);

        parsedData.push_back(WeatherItem{
            temperature: temperature,
            windSpeed: windSpeed,
            hoursAhead: hoursAhead,
            date: date,
        });
    }

    httpClient.end();

    return parsedData;
}