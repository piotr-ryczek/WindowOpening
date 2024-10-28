#ifndef BACKEND_APP_H
#define BACKEND_APP_H

#include <HTTPClient.h>

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

class BackendApp {
  private:
      HTTPClient& httpClient;

  public:
      BackendApp(HTTPClient& httpClient);

      void saveLogToApp(BackendAppLog logData);
};



#endif