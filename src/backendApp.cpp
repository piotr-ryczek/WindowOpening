#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <backendApp.h>
#include <secrets.h>

BackendApp::BackendApp(HTTPClient& httpClient): httpClient(httpClient) {}

void BackendApp::saveLogToApp(BackendAppLog logData) {
  httpClient.begin("http://window-opening.nero12.usermd.net");

  httpClient.addHeader("Content-Type", "application/json");
  httpClient.addHeader("App-Secret", BACKEND_APP_SECRET);

  JsonDocument doc; // Should be better adjusted

  doc["insideTemperature"] = logData.insideTemperature;
  doc["windowOpening"] = logData.windowOpening;

  if (logData.outsideTemperature != nullptr) {
    doc["outsideTemperature"] = *logData.outsideTemperature;
  }

  if (logData.pm25 != nullptr) {
    doc["pm25"] = *logData.pm25;
  }

  if (logData.pm10 != nullptr) {
    doc["pm10"] = *logData.pm10;
  }

  doc["config"]["weatherLogNotOlderThanHours"] = logData.config.weatherLogNotOlderThanHours;
  doc["config"]["pm25Norm"] = logData.config.pm25Norm;
  doc["config"]["pm10Norm"] = logData.config.pm10Norm;
  doc["config"]["pm25Weight"] = logData.config.pm25Weight;
  doc["config"]["pm10Weight"] = logData.config.pm10Weight;
  doc["config"]["maxOutsideTemperatureDiffFromOptimal"] = logData.config.maxOutsideTemperatureDiffFromOptimal;
  doc["config"]["outsideTemperatureClosingThreshold"] = logData.config.outsideTemperatureClosingThreshold;
  doc["config"]["optimalTemperature"] = logData.config.optimalTemperature;
  doc["config"]["pTermPositive"] = logData.config.pTermPositive;
  doc["config"]["pTermNegative"] = logData.config.pTermNegative;
  doc["config"]["dTermPositive"] = logData.config.dTermPositive;
  doc["config"]["dTermNegative"] = logData.config.dTermNegative;
  doc["config"]["oTermPositive"] = logData.config.oTermPositive;
  doc["config"]["oTermNegative"] = logData.config.oTermNegative;
  doc["config"]["iTerm"] = logData.config.iTerm;
  doc["config"]["openingTermPositiveTemperatureIncrease"] = logData.config.openingTermPositiveTemperatureIncrease;
  doc["config"]["changeDiffThreshold"] = logData.config.changeDiffThreshold;

  String payload;
  serializeJson(doc, payload);

  int httpCode = httpClient.POST(payload);

  if (httpCode == 0) {
    throw std::runtime_error("BackendApp Request failed");
  }

  if (httpCode != 200) {
    Serial.println("BackendApp didn't respond with 200");
  }
}