#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <backendApp.h>
#include <backgroundApp.h>
#include <secrets.h>
#include <config.h>
#include <timeHelpers.h>

BackendApp::BackendApp(HTTPClient* httpClient, BackgroundApp* backgroundApp): httpClient(httpClient), backgroundApp(backgroundApp) {}

void BackendApp::addHeaders() {
  httpClient->addHeader("Content-Type", "application/json");
  httpClient->addHeader("App-Secret", BACKEND_APP_SECRET);
}

void BackendApp::saveLogToApp(BackendAppLog* logData) {
   Serial.println("Saving log to Backend: Trying to query Adding Log");

  if (!httpClient->begin(BACKEND_APP_URL)) {
    Serial.println("Failed to connect to BackendApp");
    backgroundApp->addWarning(BACKEND_HTTP_REQUEST_FAILED);
    return;
  }

  backgroundApp->removeWarning(BACKEND_HTTP_REQUEST_FAILED);

  this->addHeaders();

  JsonDocument doc; // Should be better adjusted

  // Main data
  doc["insideTemperature"] = logData->insideTemperature;
  doc["windowOpening"] = logData->windowOpening;
  doc["deltaTemporaryWindowOpening"] = logData->deltaTemporaryWindowOpening;
  doc["deltaFinalWindowOpening"] = logData->deltaFinalWindowOpening;

  if (logData->outsideTemperature != nullptr) {
    doc["outsideTemperature"] = *logData->outsideTemperature;
  }

  if (logData->pm25 != nullptr) {
    doc["pm25"] = *logData->pm25;
  }

  if (logData->pm10 != nullptr) {
    doc["pm10"] = *logData->pm10;
  }

  // Config
  doc["config"]["weatherLogNotOlderThanHours"] = logData->config.weatherLogNotOlderThanHours;
  doc["config"]["pm25Norm"] = logData->config.pm25Norm;
  doc["config"]["pm10Norm"] = logData->config.pm10Norm;
  doc["config"]["pm25Weight"] = logData->config.pm25Weight;
  doc["config"]["pm10Weight"] = logData->config.pm10Weight;
  doc["config"]["maxOutsideTemperatureDiffFromOptimal"] = logData->config.maxOutsideTemperatureDiffFromOptimal;
  doc["config"]["outsideTemperatureClosingThreshold"] = logData->config.outsideTemperatureClosingThreshold;
  doc["config"]["optimalTemperature"] = logData->config.optimalTemperature;
  doc["config"]["pTermPositive"] = logData->config.pTermPositive;
  doc["config"]["pTermNegative"] = logData->config.pTermNegative;
  doc["config"]["dTermPositive"] = logData->config.dTermPositive;
  doc["config"]["dTermNegative"] = logData->config.dTermNegative;
  doc["config"]["oTermPositive"] = logData->config.oTermPositive;
  doc["config"]["oTermNegative"] = logData->config.oTermNegative;
  doc["config"]["iTerm"] = logData->config.iTerm;
  doc["config"]["openingTermPositiveTemperatureIncrease"] = logData->config.openingTermPositiveTemperatureIncrease;
  doc["config"]["changeDiffThreshold"] = logData->config.changeDiffThreshold;

  // Partial Data
  doc["partialData"]["proportionalTermValue"] = logData->partialData.proportionalTermValue;
  doc["partialData"]["integralTermValue"] = logData->partialData.integralTermValue;
  doc["partialData"]["derivativeTermValue"] = logData->partialData.derivativeTermValue;
  doc["partialData"]["openingTermValue"] = logData->partialData.openingTermValue;

  if (logData->partialData.outsideTemperatureTermValue != nullptr) {
    doc["partialData"]["outsideTemperatureTermValue"] = *logData->partialData.outsideTemperatureTermValue;
  }
  
  if (logData->partialData.airPollutionTermValue != nullptr) {
    doc["partialData"]["airPollutionTermValue"] = *logData->partialData.airPollutionTermValue;
  }
  
  Serial.println("Saving log to Backend: Serialize JSON");

  String payload;
  serializeJson(doc, payload);

  int httpCode = httpClient->POST(payload);

  if (httpCode <= 0) {
    Serial.println("BackendApp Request failed");
    backgroundApp->addWarning(BACKEND_HTTP_REQUEST_FAILED);
  }

  if (httpCode != 201) {
    Serial.println("BackendApp didn't respond with 201");
    backgroundApp->addWarning(BACKEND_HTTP_REQUEST_FAILED);
  } else {
    Serial.println("BackendApp retrieved log data");
    backgroundApp->removeWarning(BACKEND_HTTP_REQUEST_FAILED);
  }

  Serial.println("Saving log to Backend: Query finished");

  httpClient->end();
}

vector<WeatherItem> BackendApp::fetchWeatherForecast() {
  Serial.println("Trying to query Weather Forecast");

  vector<WeatherItem> parsedData;

  if (!httpClient->begin(String(BACKEND_APP_URL) + "/weather-forecast")) {
    Serial.println("Failed to connect to BackendApp");
    backgroundApp->addWarning(WEATHER_FORECAST_HTTP_REQUEST_FAILED);
    return parsedData;
  }

  this->addHeaders();

  int httpCode = httpClient->GET();

  if (httpCode != 200) {
      Serial.println("WeatherForecast didn't respond with 200");
      backgroundApp->addWarning(WEATHER_FORECAST_HTTP_REQUEST_FAILED);
      return parsedData;
  }

  backgroundApp->removeWarning(WEATHER_FORECAST_HTTP_REQUEST_FAILED);

  String response = httpClient->getString();

  Serial.println("WeatherForecast: Deserialize JSON");

  JsonDocument doc;
  deserializeJson(doc, response);

  for (const auto& item: doc.as<JsonArray>()) {
      float temperature = item["temperature"];
      float windSpeed = item["windSpeed"];
      String date = item["date"];
      float hoursAhead = calculateHoursAhead(date);

      if (hoursAhead != false) {
        parsedData.push_back(WeatherItem{
          temperature: temperature,
          windSpeed: windSpeed,
          hoursAhead: hoursAhead,
          date: date,
      });
      }
  }
  
  httpClient->end();

  Serial.println("WeatherForecast: Query finished");

  return parsedData;
}

AirPollutionItem BackendApp::fetchAirPollution() {
  Serial.println("Trying to query AirPollution");

  if (!httpClient->begin(String(BACKEND_APP_URL) + "/air-pollution")) {
    Serial.println("Failed to connect to BackendApp");
    backgroundApp->addWarning(AIR_POLLUTION_HTTP_REQUEST_FAILED);
    return AirPollutionItem{};
  }

  this->addHeaders();

  int httpCode = httpClient->GET();

  if (httpCode != 200) {
      Serial.println("PM Request didn't respond with 200");
      backgroundApp->addWarning(AIR_POLLUTION_HTTP_REQUEST_FAILED);
      return AirPollutionItem{};
  }

  backgroundApp->removeWarning(AIR_POLLUTION_HTTP_REQUEST_FAILED);

  String response = httpClient->getString();

  Serial.println("AirPollution: Deserialize JSON");

  JsonDocument doc;
  deserializeJson(doc, response);

  SensorResponseItem pm25Result;
  pm25Result.value = doc["pm25"][0]["value"];
  String pm25Date = doc["pm25"][0]["date"];
  pm25Result.date = pm25Date;

  SensorResponseItem pm10Result;
  pm10Result.value = doc["pm10"][0]["value"];
  String pm10Date = doc["pm10"][0]["date"];
  pm10Result.date = pm10Date;

  httpClient->end();

  Serial.println("AirPollution: Query finished");

  return AirPollutionItem{
    pm25: pm25Result.value,
    pm25Date: pm25Result.date,
    pm10: pm10Result.value,
    pm10Date: pm10Result.date 
  };
}