#ifndef BACKGROUND_APP_H
#define BACKGROUND_APP_H

#include <Arduino.h>
#include <set>
#include <vector>
#include <ledWrapper.h>
#include <weatherForecast.h>
#include <lcdWrapper.h>

using namespace std;

const int timerDelay = 500; // For switching LED Color
const int WARNING_WEATHER_MAX_HOURS_AHEAD = 9; // How many hours ahead are we going to check weather
const int WARNING_WIND_SPEED = 8; // m/s

// LOW_BATTERY - currently unused
enum WarningEnum { NONE_WARNING, LOW_BATTERY, WEATHER_DANGEROUS, WIFI_FAILED, BACKEND_HTTP_REQUEST_FAILED, WEATHER_FORECAST_HTTP_REQUEST_FAILED, AIR_POLLUTION_HTTP_REQUEST_FAILED };

class BackgroundApp {
    private:
        LedWrapper& led;
        LcdWrapper& lcd;
        long lastWarningChangeTimer;
        boolean isLedActive;
        set<WarningEnum>::iterator currentWarningDisplayedIndex;

        void displayLedColorByWarning(WarningEnum warning);
        String translateWarningEnumToString(WarningEnum warning);

    public:
        set<WarningEnum> warnings;
        
        BackgroundApp(LedWrapper& led, LcdWrapper& lcd);

        void addWarning(WarningEnum warning);
        void removeWarning(WarningEnum warning);
        void checkForWeatherWarning(vector<WeatherItem> weatherItems);

        void handleWarningsDisplay();
};

#endif