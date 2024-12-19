#ifndef BACKEND_APP_H
#define BACKEND_APP_H

#include <vector>
#include <HTTPClient.h>

class BackgroundApp;

using namespace std;

struct BackendAppLogConfig {
    double weatherLogNotOlderThanHours;
    double pm25Norm;
    double pm10Norm;
    double pm25Weight;
    double pm10Weight;
    double maxOutsideTemperatureDiffFromOptimal;
    double outsideTemperatureClosingThreshold;
    double optimalTemperature;
    double pTermPositive;
    double pTermNegative;
    double dTermPositive;
    double dTermNegative;
    double oTermPositive;
    double oTermNegative;
    double iTerm;
    double openingTermPositiveTemperatureIncrease;
    double changeDiffThreshold;
};


struct BackendAppLogPartialData {
    double proportionalTermValue;
    double integralTermValue;
    double derivativeTermValue;
    double openingTermValue;
    double* outsideTemperatureTermValue; // Optional
    double* airPollutionTermValue; // Optional
};

struct BackendAppLog {
    int windowOpening;
    int deltaTemporaryWindowOpening; // Before taking into account change threshold
    int deltaFinalWindowOpening;
    double insideTemperature;
    double* outsideTemperature; // Optional
    double* pm25; // Optional
    double* pm10; // Optional
    BackendAppLogConfig config;
    BackendAppLogPartialData partialData;
};

struct WeatherItem {
    float temperature;
    float windSpeed;
    float hoursAhead;
    String date;
};

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

struct SensorResponseItem {
    String date;
    double value;
};

class BackendApp {
  private:
      HTTPClient* httpClient;
      BackgroundApp* backgroundApp;

      void addHeaders();

  public:
      BackendApp(HTTPClient* httpClient, BackgroundApp* backgroundApp);

      void saveLogToApp(BackendAppLog* logData);
      vector<WeatherItem> fetchWeatherForecast();
      AirPollutionItem fetchAirPollution();
};



#endif