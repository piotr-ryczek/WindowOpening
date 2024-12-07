#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <backendApp.h>
#include <secrets.h>

BackendApp::BackendApp(HTTPClient* httpClient, BackgroundApp* backgroundApp): httpClient(httpClient), backgroundApp(backgroundApp) {}

void BackendApp::saveLogToApp(BackendAppLog logData) {
  if (!httpClient->begin("https://window-opening.nero12.usermd.net")) {
    Serial.println("Failed to connect to BackendApp");
    return;
  }

  httpClient->addHeader("Content-Type", "application/json");
  httpClient->addHeader("App-Secret", BACKEND_APP_SECRET);

  JsonDocument doc; // Should be better adjusted

  // Main data
  doc["insideTemperature"] = logData.insideTemperature;
  doc["windowOpening"] = logData.windowOpening;
  doc["deltaTemporaryWindowOpening"] = logData.deltaTemporaryWindowOpening;
  doc["deltaFinalWindowOpening"] = logData.deltaFinalWindowOpening;

  if (logData.outsideTemperature != nullptr) {
    doc["outsideTemperature"] = *logData.outsideTemperature;
  }

  if (logData.pm25 != nullptr) {
    doc["pm25"] = *logData.pm25;
  }

  if (logData.pm10 != nullptr) {
    doc["pm10"] = *logData.pm10;
  }

  // Config
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

  // Partial Data
  doc["partialData"]["proportionalTermValue"] = logData.partialData.proportionalTermValue;
  doc["partialData"]["integralTermValue"] = logData.partialData.integralTermValue;
  doc["partialData"]["derivativeTermValue"] = logData.partialData.derivativeTermValue;
  doc["partialData"]["openingTermValue"] = logData.partialData.openingTermValue;

  if (logData.partialData.outsideTemperatureTermValue != nullptr) {
    doc["partialData"]["outsideTemperatureTermValue"] = *logData.partialData.outsideTemperatureTermValue;
  }
  
  if (logData.partialData.airPollutionTermValue != nullptr) {
    doc["partialData"]["airPollutionTermValue"] = *logData.partialData.airPollutionTermValue;
  }
  

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

  httpClient->end();
}