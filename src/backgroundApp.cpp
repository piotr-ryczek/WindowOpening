#include <backgroundApp.h>
#include <Arduino.h>

BackgroundApp::BackgroundApp(LedWrapper& led, LcdWrapper& lcd): led(led), lcd(lcd) {
    this->lastWarningChangeTimer = millis();
    this->currentWarningDisplayedIndex = this->warnings.begin();
    this->isLedActive = false;
}

void BackgroundApp::addWarning(WarningEnum warning) {
    warnings.insert(warning);
}

void BackgroundApp::removeWarning(WarningEnum warning) {
    warnings.erase(warning);
}

void BackgroundApp::displayLedColorByWarning(WarningEnum warning) {
    switch (warning) {
        case LOW_BATTERY:
            led.setColorPurple();
            break;

        case WEATHER_DANGEROUS:
            led.setColorRed();
            break;

        case WIFI_FAILED:
            led.setColorYellow(); // Same
            break;

        case BACKEND_HTTP_REQUEST_FAILED:
        case WEATHER_FORECAST_HTTP_REQUEST_FAILED:
        case AIR_POLLUTION_HTTP_REQUEST_FAILED:
            led.setColorYellow(); // Same
            break;

        case NONE_WARNING:
        default:
            led.setNoColor();
    }
}

void BackgroundApp::handleWarningsDisplay() {
    long currentMillis = millis();

    if (currentMillis - lastWarningChangeTimer < timerDelay) return;

    lastWarningChangeTimer = currentMillis;

    if (warnings.empty()) {
        displayLedColorByWarning(NONE_WARNING);
        return;
    }

    isLedActive = !isLedActive;

    if (isLedActive) {
        currentWarningDisplayedIndex++;

        if (currentWarningDisplayedIndex == warnings.end()) {
            currentWarningDisplayedIndex = warnings.begin();
        }

        displayLedColorByWarning(*currentWarningDisplayedIndex);
        this->lcd.backlight();
        this->lcd.print(translateWarningEnumToString(*currentWarningDisplayedIndex));
    } else {
        displayLedColorByWarning(NONE_WARNING);
        this->lcd.clear();
        this->lcd.noBacklight();
    }
}

void BackgroundApp::checkForWeatherWarning(vector<WeatherItem> weatherItems) {
    for (const auto &item : weatherItems) {
        if (item.hoursAhead < WARNING_WEATHER_MAX_HOURS_AHEAD && item.windSpeed > WARNING_WIND_SPEED) {
            this->addWarning(WEATHER_DANGEROUS);
            return;
        }
    }

    // In case data has been checked and nothing dangerous found
    this->removeWarning(WEATHER_DANGEROUS);
}

void BackgroundApp::clearWarnings() {
    this->removeWarning(NONE_WARNING);
    this->removeWarning(LOW_BATTERY);
    this->removeWarning(WEATHER_DANGEROUS);
    this->removeWarning(WIFI_FAILED);
    this->removeWarning(BACKEND_HTTP_REQUEST_FAILED);
    this->removeWarning(WEATHER_FORECAST_HTTP_REQUEST_FAILED);
    this->removeWarning(AIR_POLLUTION_HTTP_REQUEST_FAILED);
}

String BackgroundApp::translateWarningEnumToString(WarningEnum warning) {
    switch (warning) {
        case LOW_BATTERY:
            return "LOW BATTERY";

        case WEATHER_DANGEROUS:
            return "WEATHER DANGEROUS";

        case WIFI_FAILED:
            return "WIFI FAILED";

        case BACKEND_HTTP_REQUEST_FAILED:
            return "BACKEND FAILED";

        case WEATHER_FORECAST_HTTP_REQUEST_FAILED:
            return "WEATHER FORECAST FAILED" ;

        case AIR_POLLUTION_HTTP_REQUEST_FAILED:
            return "AIR POLLUTION FAILED";

        case NONE_WARNING:
        default:
            return "NONE";
    }
}