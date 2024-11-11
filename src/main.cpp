#include <Arduino.h>
#include <EEPROM.h>
#include <vector>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <LiquidCrystal_I2C.h>

#include <gpios.h>
#include <memoryData.h>
#include <buttonHandler.h>
#include <navigation.h>
#include <servoWrapper.h>
#include <ledWrapper.h>
#include <config.h>
#include <backgroundApp.h>
#include <weatherForecast.h>
#include <airPollution.h>
#include <secrets.h>
#include <pidController.h>
#include <logs.h>
#include <weatherLogs.h>
#include <backendApp.h>
#include <lcdWrapper.h>

/**
 * How to simulate calculations:
 * AppModeEnum AppMode = Manual; -> Auto
 * float currentTemperature = bme.readTemperature(); -> hardcode temperature
 */

/**
 * Next:
 * - None
 * 
 * Further next:
 * - WebApp to aggregate data
 * - Bluetooth Integration
 * - Speaker implementation for Alerts
 */

#define EEPROM_SIZE 64
#define BME280_ADDRESS 0x76

using namespace std;

bool isWifiConnected = false;

const int GMT_OFFSET_SEC = 3600;
const int DAYLIGHT_OFFSET_SEC = 3600;
const char* NTP_SERVER_URL = "pool.ntp.org";

TwoWire I2C_BME_280 = TwoWire(1);

const int MOVE_SMOOTHLY_MILISECONDS_INTERVAL = 40;
const int WINDOW_OPENING_CALCULATION_INTERVAL = 1000 * 60 * 5; // Every 5 minutes

const int AIR_POLLUTION_SENSOR_PM_25_ID = 2071;
const int AIR_POLLUTION_SENSOR_PM_10_ID = 2069;
const char* AIR_POLLUTION_SENSOR_API_URL = "https://api.gios.gov.pl/pjp-api/rest/data/getData/";
const char* WEATHER_FORECAST_API_URL = "https://api.openweathermap.org/data/2.5/";

const double LOCATION_LAT = 51.760229;
const double LOCATION_LON = 19.550675;

const int DELAY_DIFF_BETWEEN_SERVOS_MILISECONDS = 500; // 0.5s - difference between one will start before other one

TaskHandle_t NavigationTask;
TaskHandle_t WarningsTask;
TaskHandle_t ServosSmoothMovementTask;
TaskHandle_t WifiConnectionTask;
TaskHandle_t WeatherConnectionTask;
TaskHandle_t WeatherForecastTask;
TaskHandle_t WindowOpeningCalculationTask;
TaskHandle_t DisplayTask;

// Instances

LiquidCrystal_I2C lcd(0x27, 16, 2);
LcdWrapper lcdWrapper(&lcd);

HTTPClient httpClient;

Servo servoPullOpen;
Servo servoPullClose;
ServoWrapper servoPullOpenWrapper(SERVO_PULL_OPEN_GPIO, servoPullOpen, servoPullOpenCalibrationMinMemory, servoPullOpenCalibrationMaxMemory);
ServoWrapper servoPullCloseWrapper(SERVO_PULL_CLOSE_GPIO, servoPullClose, servoPullCloseCalibrationMinMemory, servoPullCloseCalibrationMaxMemory);

LedWrapper ledWrapper(LED_RED_PWM_TIMER_INDEX, LED_RED_GPIO, LED_GREEN_PWM_TIMER_INDEX, LED_GREEN_GPIO, LED_BLUE_PWM_TIMER_INDEX, LED_BLUE_GPIO);

Navigation navigation(POTENTIOMETER_GPIO, servoPullOpenWrapper, servoPullCloseWrapper, ledWrapper, &AppMode, &lcdWrapper);
ButtonHandler enterButton(ENTER_BUTTON_GPIO);
ButtonHandler exitButton(EXIT_BUTTON_GPIO);

BackgroundApp backgroundApp(ledWrapper);
BackendApp backendApp(httpClient);

WeatherForecast weatherForecast(httpClient, WEATHER_FORECAST_API_URL, WEATHER_FORECAST_API_KEY, LOCATION_LAT, LOCATION_LON);
AirPollution airPollution(httpClient, AIR_POLLUTION_SENSOR_API_URL, AIR_POLLUTION_SENSOR_PM_25_ID, AIR_POLLUTION_SENSOR_PM_10_ID);

Adafruit_BME280 bme;

void handleEnterButtonPress() {
    navigation.handleForward();
};

void handleExitButtonPress() {
    navigation.handleBackward();
};

void navigationTask(void *param) {
    while (true) {
        enterButton.checkButtonPress();
        exitButton.checkButtonPress();

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void warningsTask(void *param) {
    while (true) {
        if (navigation.appMainState == Sleep) {
            backgroundApp.handleWarningsDisplay();
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void servosSmoothMovementTask(void *param) {
    while (true) {
        if (servoPullOpenWrapper.isMovingSmoothly) {
            servoPullOpenWrapper.moveSmoothly();
        }

        if (servoPullCloseWrapper.isMovingSmoothly) {
            servoPullCloseWrapper.moveSmoothly();
        }

        vTaskDelay(MOVE_SMOOTHLY_MILISECONDS_INTERVAL / portTICK_PERIOD_MS);
    }
}

void windowOpeningCalculationTask(void *param) {
    while (true) {
        if (AppMode == Auto) {
            float currentTemperature = bme.readTemperature();
            auto [newWindowOpening, backendAppLog] = PIDController::calculateWindowOpening(currentTemperature);

            // Save to Backend
            if (isWifiConnected) {
                backendApp.saveLogToApp(backendAppLog);
            }

            Log lastLog = logs.back();

            if (newWindowOpening != lastLog.windowOpening) {
                boolean isWindowOpening = newWindowOpening > lastLog.windowOpening; // Closing or Opening

                if (isWindowOpening) {
                    servoPullCloseWrapper.setMovingSmoothlyTarget(newWindowOpening);
                    vTaskDelay(DELAY_DIFF_BETWEEN_SERVOS_MILISECONDS);
                    servoPullOpenWrapper.setMovingSmoothlyTarget(newWindowOpening);
                } else {
                    servoPullOpenWrapper.setMovingSmoothlyTarget(newWindowOpening);
                    vTaskDelay(DELAY_DIFF_BETWEEN_SERVOS_MILISECONDS);
                    servoPullCloseWrapper.setMovingSmoothlyTarget(newWindowOpening);
                }
            }

            // By default every 5 minutes
            vTaskDelay(WINDOW_OPENING_CALCULATION_INTERVAL / portTICK_PERIOD_MS);
        }

        // Checking if state has changed every second
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void weatherForecastTask(void *param) {
    while (true) {
        if (!isWifiConnected && !WEATHER_FORECAST_ENABLED) return;

        auto weatherItems = weatherForecast.fetchData();
        WeatherItem weatherItem = weatherItems.front();
        backgroundApp.checkForWeatherWarning(weatherItems);        

        auto airPollutionData = airPollution.fetchData();

        addWeatherLog(weatherItem.temperature, weatherItem.date, airPollutionData.pm25, airPollutionData.pm25Date, airPollutionData.pm10, airPollutionData.pm10Date);

        vTaskDelay(1000 * 60 * 60 / portTICK_PERIOD_MS); // Once per hour
    }
}

void wifiConnectionTask(void *param) {
    while (true) {
        auto wifiStatus = WiFi.status();
        
        if (wifiStatus == WL_CONNECTED) {
            isWifiConnected = true;
            Serial.println("WiFi Connected");
            configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER_URL); // Synchronize time
            vTaskDelay(5000 / portTICK_PERIOD_MS); // Delay for 5s to give a time to retrieve current time (for first time)

            xTaskCreate(weatherForecastTask, "WeatherForecastTask", 16384, NULL, 5, &WeatherForecastTask);

            vTaskSuspend(WifiConnectionTask);
        } else {
            isWifiConnected = false;
            Serial.println("WiFi Connecting");
            Serial.print("WiFi Status:");
            Serial.println(wifiStatus);
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS); // Once per second
    }
}

void displayTask(void *param) {
    while (true) {
        lcdWrapper.checkScroll();

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void setup() {
    Wire.begin(LCD_SDA_GPIO, LCD_SCL_GPIO);
    Serial.begin(115200);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    if (!EEPROM.begin(EEPROM_SIZE)) {
        Serial.println("EEPROM Error");
        return;
    }

    I2C_BME_280.begin(BME_280_SDA_GPIO, BME_280_SCL_GPIO, 100000); 

    if (!bme.begin(BME280_ADDRESS, &I2C_BME_280)) {
        Serial.println("BME280 not working correctly");
        while (1);
    }

    delay(100);

    // Init first log (50 will be invalid value probably)
    float initialTemperature = bme.readTemperature();
    addLog(initialTemperature, 50);

    ledWrapper.initialize();

    enterButton.initialize();
    exitButton.initialize();
    enterButton.attachButtonPressCallback(handleEnterButtonPress);
    exitButton.attachButtonPressCallback(handleExitButtonPress);

    servoPullOpenWrapper.initialize(SERVO_PULL_OPEN_PWM_TIMER_INDEX);
    servoPullCloseWrapper.initialize(SERVO_PULL_CLOSE_PWM_TIMER_INDEX);

    xTaskCreate(navigationTask, "NavigationTask", 2048, NULL, 1, &NavigationTask);
    xTaskCreate(warningsTask, "WarningsTask", 2048, NULL, 2, &WarningsTask);
    xTaskCreate(servosSmoothMovementTask, "ServosSmoothMovementTask", 2048, NULL, 3, &ServosSmoothMovementTask);
    xTaskCreate(wifiConnectionTask, "WifiConnectionTask", 2048, NULL, 4, &WifiConnectionTask);
    xTaskCreate(windowOpeningCalculationTask, "WindowOpeningCalculationTask", 16384, NULL, 5, &WindowOpeningCalculationTask);
    xTaskCreate(displayTask, "displayTask", 2048, NULL, 6, &DisplayTask);

    lcdWrapper.init();
}

void loop() {
    if (navigation.isMenuSelectionActivated) {
        navigation.handleMenuSelection();
    }

    switch (navigation.mainMenuState) {
        case MainMenuServoSelection:
            navigation.handleServoSelection();
            break;
        case MainMenuCalibration:
            navigation.handleCalibrate();
            break;
        case MainMenuMove:
            navigation.handleMove();
            break;
        case MainMenuMoveBothServos:
            navigation.handleMoveBothServos();
            break;
        case MainMenuAppMode:
            navigation.handleAppModeSelection();
            break;
        case MainMenuSettings: {
            if (navigation.selectedSetting == nullptr) {
                navigation.handleSettingSelection();
            } else {
                navigation.handleSetSettingValue();
            }

            break;
        }
        case MainMenuMoveSmoothly: {
            navigation.handleMoveSmoothlySelection();
            break;
        }
        case MainMenuMoveBothServosSmoothly: {
            navigation.handleMoveBothServosSmoothlySelection();
            break;
        }
    }

    delay(20);
}