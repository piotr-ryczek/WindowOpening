#include <backgroundApp.h>
#include <Arduino.h>

BackgroundApp::BackgroundApp(LedWrapper& led, LcdWrapper& lcd, MemoryValue* warningsAreActiveMemory): led(led), lcd(lcd), warningsAreActiveMemory(warningsAreActiveMemory) {
    this->lastWarningChangeTimer = millis();
    this->currentWarningDisplayedIndex = this->warnings.begin();
    this->isLedActive = false;
}

void BackgroundApp::addWarning(WarningEnum warning) {
    warnings.insert(warning);
}

void BackgroundApp::removeWarning(WarningEnum warning) {
    auto it = warnings.find(warning);
    if (it != warnings.end()) {
        auto nextIt = next(it);

        warnings.erase(it);

        if (nextIt != warnings.end()) {
            currentWarningDisplayedIndex = nextIt;
        } else {
            currentWarningDisplayedIndex = warnings.begin();
        }
    }
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
    if (warningsAreActiveMemory->readValue() == 0) {
        return;
    }

    long currentMillis = millis();

    if (currentMillis - lastWarningChangeTimer < timerDelay) return;

    lastWarningChangeTimer = currentMillis;

    if (warnings.empty()) {
        this->lcd.clear();
        this->lcd.noBacklight();
        displayLedColorByWarning(NONE_WARNING);
        return;
    }

    isLedActive = !isLedActive;

    if (isLedActive) {
         if (warnings.find(*currentWarningDisplayedIndex) == warnings.end()) {
            currentWarningDisplayedIndex = warnings.begin();
        } else {
            currentWarningDisplayedIndex++;
            if (currentWarningDisplayedIndex == warnings.end()) {
                currentWarningDisplayedIndex = warnings.begin();
            }
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
    if (weatherItems.empty()) {
        return;
    }

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
    warnings.clear();
    currentWarningDisplayedIndex = warnings.begin();
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