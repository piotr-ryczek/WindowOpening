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
#include <buttonHandler.h>
#include <navigation.h>
#include <servoWrapper.h>
#include <ledWrapper.h>
#include <config.h>
#include <backgroundApp.h>
#include <memoryValue.h>
#include <weatherForecast.h>
#include <airPollution.h>
#include <secrets.h>
#include <pidController.h>
#include <logs.h>
#include <weatherLogs.h>

/**
 * Next:
 * - Speaker implementation for Alerts
 * - LCD Screen implementation
 * 
 * Further next:
 * - WebApp to aggregate data
 */

#define EEPROM_SIZE 64
#define BME280_ADDRESS 0x76

using namespace std;

const int GMT_OFFSET_SEC = 3600;
const int DAYLIGHT_OFFSET_SEC = 3600;
const char* NTP_SERVER_URL = "pool.ntp.org";

const int LED_RED_PWM_TIMER_INDEX = 0;
const int LED_GREEN_PWM_TIMER_INDEX = 1;
const int LED_BLUE_PWM_TIMER_INDEX = 2;

const int SERVO_PULL_OPEN_PWM_TIMER_INDEX = 3;
const int SERVO_PULL_CLOSE_PWM_TIMER_INDEX = 4;

const byte ENTER_BUTTON_GPIO = 14;
const byte EXIT_BUTTON_GPIO = 12;
const byte POTENTIOMETER_GPIO = 34;
const byte SERVO_PULL_OPEN_GPIO = 21;
const byte SERVO_PULL_CLOSE_GPIO = 22;

const byte LED_RED_GPIO = 25;
const byte LED_GREEN_GPIO = 26;
const byte LED_BLUE_GPIO = 27;

const int SERVO_PULL_OPEN_CALIBRATION_MIN_SET_ADDRESS = 0;
const int SERVO_PULL_OPEN_CALIBRATION_MIN_VALUE_ADDRESS = 1;

const int SERVO_PULL_OPEN_CALIBRATION_MAX_SET_ADDRESS = 2;
const int SERVO_PULL_OPEN_CALIBRATION_MAX_VALUE_ADDRESS = 3;

const int SERVO_PULL_CLOSE_CALIBRATION_MIN_SET_ADDRESS = 4;
const int SERVO_PULL_CLOSE_CALIBRATION_MIN_VALUE_ADDRESS = 5;

const int SERVO_PULL_CLOSE_CALIBRATION_MAX_SET_ADDRESS = 6;
const int SERVO_PULL_CLOSE_CALIBRATION_MAX_VALUE_ADDRESS = 7;

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

// Pull Open Calibration Min
MemoryValue servoPullOpenCalibrationMinMemory(SERVO_PULL_OPEN_CALIBRATION_MIN_SET_ADDRESS, SERVO_PULL_OPEN_CALIBRATION_MIN_VALUE_ADDRESS);

// Pull Open Calibration  Max
MemoryValue servoPullOpenCalibrationMaxMemory(SERVO_PULL_OPEN_CALIBRATION_MAX_SET_ADDRESS, SERVO_PULL_OPEN_CALIBRATION_MAX_VALUE_ADDRESS);

// Pull Close Calibration Min
MemoryValue servoPullCloseCalibrationMinMemory(SERVO_PULL_CLOSE_CALIBRATION_MIN_SET_ADDRESS, SERVO_PULL_CLOSE_CALIBRATION_MIN_VALUE_ADDRESS);

// Pull Close Calibration Max
MemoryValue servoPullCloseCalibrationMaxMemory(SERVO_PULL_CLOSE_CALIBRATION_MAX_SET_ADDRESS, SERVO_PULL_CLOSE_CALIBRATION_MAX_VALUE_ADDRESS);

HTTPClient httpClient;

Servo servoPullOpen;
Servo servoPullClose;
ServoWrapper servoPullOpenWrapper(SERVO_PULL_OPEN_GPIO, servoPullOpen, servoPullOpenCalibrationMinMemory, servoPullOpenCalibrationMaxMemory);
ServoWrapper servoPullCloseWrapper(SERVO_PULL_CLOSE_GPIO, servoPullClose, servoPullCloseCalibrationMinMemory, servoPullCloseCalibrationMaxMemory);

LedWrapper ledWrapper(LED_RED_PWM_TIMER_INDEX, LED_RED_GPIO, LED_GREEN_PWM_TIMER_INDEX, LED_GREEN_GPIO, LED_BLUE_PWM_TIMER_INDEX, LED_BLUE_GPIO);

Navigation navigation(POTENTIOMETER_GPIO, servoPullOpenWrapper, servoPullCloseWrapper, ledWrapper, &AppMode);
ButtonHandler enterButton(ENTER_BUTTON_GPIO);
ButtonHandler exitButton(EXIT_BUTTON_GPIO);

BackgroundApp backgroundApp(ledWrapper);

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
            int newWindowOpening = PIDController::calculateWindowOpening(currentTemperature);

            Log lastLog = logs.back();

            if (newWindowOpening != lastLog.windowOpening) {
                boolean isWindowOpening = newWindowOpening > lastLog.windowOpening;

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
        }

        vTaskDelay(WINDOW_OPENING_CALCULATION_INTERVAL / portTICK_PERIOD_MS);
    }
}

void weatherForecastTask(void *param) {
    while (true) {
        
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
            Serial.println("WiFi Connected");
            configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER_URL); // Synchronize time
            vTaskDelay(5000 / portTICK_PERIOD_MS); // Delay for 5s to give a time to retrieve current time (for first time)

            xTaskCreate(weatherForecastTask, "WeatherForecastTask", 16384, NULL, 5, &WeatherForecastTask);
            vTaskSuspend(WifiConnectionTask);
        } else {
            Serial.println("WiFi Connecting");
            Serial.print("WiFi Status:");
            Serial.println(wifiStatus);
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS); // Once per second
    }
}

void setup() {
    Serial.begin(115200);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    if (!EEPROM.begin(EEPROM_SIZE)) {
        Serial.println("EEPROM Error");
        return;
    }

    if (!bme.begin(BME280_ADDRESS)) {
        Serial.println("BME280 not working correctly");
        while (1);
    }

    delay(100);

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
    xTaskCreate(windowOpeningCalculationTask, "WindowOpeningCalculationTask", 2048, NULL, 4, &WindowOpeningCalculationTask); 
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
    }
}