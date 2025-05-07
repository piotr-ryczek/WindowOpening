#include <Arduino.h>
#include <EEPROM.h>
#include <vector>
#include <ESP32Servo.h>
// #include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <LiquidCrystal_I2C.h>
#include <WiFiClientSecure.h>

#include <gpios.h>
#include <memoryData.h>
#include <buttonHandler.h>
#include <navigation.h>
#include <servoWrapper.h>
#include <ledWrapper.h>
#include <config.h>
#include <backgroundApp.h>
#include <secrets.h>
#include <pidController.h>
#include <logs.h>
#include <weatherLogs.h>
#include <backendApp.h>
#include <lcdWrapper.h>
#include <bluetoothWrapper.h>
#include <batteryVoltageMeter.h>
#include <timeHelpers.h>
#include <periodicalTasksQueue.h>
#include <valuesJitterFilter.h>
#include <servosPowerSupply.h>
/**
 * How to simulate calculations:
 * AppModeEnum AppMode = Manual; -> Auto
 * float currentTemperature = bme.readTemperature(); -> hardcode temperature
 */

#define EEPROM_SIZE 768
#define BME280_ADDRESS 0x76

using namespace std;

TwoWire I2C_BME_280 = TwoWire(1);

// Task Handles
TaskHandle_t CheckPeriodicalTasksQueue;
TaskHandle_t ServosSmoothMovementTask;
TaskHandle_t WindowOpeningCalculationTask;
TaskHandle_t NTPTask;
// Optional
TaskHandle_t CheckMemoryTask;

const int CHECK_PERIODICAL_TASKS_QUEUE_TASK_STACK_SIZE = 14336;
const int SERVOS_SMOOTH_MOVEMENT_TASK_STACK_SIZE = 1536;
const int WINDOW_OPENING_CALCULATION_TASK_STACK_SIZE = 4096;
const int NTP_TASK_STACK_SIZE = 3072;
const int CHECK_MEMORY_TASK_STACK_SIZE = 4096;

// Instances

ValuesJitterFilter valuesJitterFilter;

LiquidCrystal_I2C lcd(0x27, 16, 2);
LcdWrapper lcdWrapper(&lcd);

HTTPClient httpClient;
WiFiClientSecure *client = new WiFiClientSecure;

BatteryVoltageMeter batteryVoltageMeterBox(BATTERY_VOLTAGE_BOX_METER_GPIO, BATTERY_VOLTAGE_0_REFERENCE, BATTERY_VOLTAGE_100_REFERENCE, lastReadBatteryVoltageBox);
BatteryVoltageMeter batteryVoltageMeterServos(BATTERY_VOLTAGE_SERVOS_METER_GPIO, BATTERY_VOLTAGE_0_REFERENCE, BATTERY_VOLTAGE_100_REFERENCE, lastReadBatteryVoltageServos);

ServosPowerSupply servosPowerSupply(SERVOS_POWER_SUPPLY_GPIO);

Servo servoPullOpen;
Servo servoPullClose;
ServoWrapper servoPullOpenWrapper(SERVO_PULL_OPEN_GPIO, servoPullOpen, servoPullOpenCalibrationMinMemory, servoPullOpenCalibrationMaxMemory, servosPowerSupply);
ServoWrapper servoPullCloseWrapper(SERVO_PULL_CLOSE_GPIO, servoPullClose, servoPullCloseCalibrationMinMemory, servoPullCloseCalibrationMaxMemory, servosPowerSupply);

LedWrapper ledWrapper(LED_RED_PWM_TIMER_INDEX, LED_RED_GPIO, LED_GREEN_PWM_TIMER_INDEX, LED_GREEN_GPIO, LED_BLUE_PWM_TIMER_INDEX, LED_BLUE_GPIO);

Navigation navigation(POTENTIOMETER_GPIO, true, servoPullOpenWrapper, servoPullCloseWrapper, ledWrapper, &AppMode, &lcdWrapper, &batteryVoltageMeterBox, &batteryVoltageMeterServos, &valuesJitterFilter);
ButtonHandler enterButton(ENTER_BUTTON_GPIO);
ButtonHandler exitButton(EXIT_BUTTON_GPIO);

BackgroundApp backgroundApp(ledWrapper, lcdWrapper, &warningsAreActiveMemory);
BackendApp backendApp(&httpClient, &backgroundApp);

Adafruit_BME280 bme;

BluetoothWrapper bluetoothWrapper(&bme, &backgroundApp, &servoPullOpenWrapper, &servoPullCloseWrapper, &batteryVoltageMeterBox, &batteryVoltageMeterServos);

enum HttpQueryTypeEnum { BackendAppWeatherForecastAndAirPollutionQueries, BackendAppSaveLogQuery };

struct HttpQueryQueueItem {
    HttpQueryTypeEnum type;
    BackendAppLog* backendAppLog;
};

bool isHttpQueriesQueueOccupied = false;

vector<HttpQueryQueueItem> httpQueriesQueue;

void initWifi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    shouldTryToConnectToWifi = true;
}

void disconnectWifi() {
    bool hasDisconnected = WiFi.disconnect();
    // WiFi.mode(WIFI_OFF);

    if (!hasDisconnected) {
        Serial.println("WiFi has not disconnected correctly");
    } else {
        isWifiConnected = false;
        Serial.println("WiFi disconnected");
    }
}

void handleEnterButtonPress() {
    navigation.handleForward();
};

void handleExitButtonPress() {
    navigation.handleBackward();
};

// Task Functions
void warningsTaskFunction() {
    if (shouldDisplayFunctionTasksExecutionLogs) {
        Serial.println(">>> warningsTaskFunction executed");
    }

    if (navigation.appMainState == Sleep) {
        backgroundApp.handleWarningsDisplay();
    }

    addPeriodicalTaskInMillis(warningsTaskFunction, 100);
}

void weatherForecastAndAirPollutionTaskFunction() {
    if (shouldDisplayFunctionTasksExecutionLogs) {
        Serial.println(">>> weatherForecastAndAirPollutionTaskFunction executed");
    }

    Serial.println("Adding to queue: BackendAppWeatherForecastAndAirPollutionQueries");
    HttpQueryQueueItem weatherForecastAndAirPollutionQueueItem = {
        type: BackendAppWeatherForecastAndAirPollutionQueries,
        backendAppLog: nullptr
    };

    httpQueriesQueue.push_back(weatherForecastAndAirPollutionQueueItem);

    addPeriodicalTaskInMillis(weatherForecastAndAirPollutionTaskFunction, 1000 * 60 * 60); // Once per hour
}

// unsigned long lastHttpRequestMillis = 0;
void httpTaskFunction() {
    if (shouldDisplayFunctionTasksExecutionLogs) {
        Serial.println(">>> httpTaskFunction executed");
    }

    if (!httpQueriesQueue.empty()) {
        if (isHttpQueriesQueueOccupied) {
            addPeriodicalTaskInMillis(httpTaskFunction, 1000);
            return;
        }

        if (!isWifiConnected && !isWifiConnecting) {
            initWifi();
            addPeriodicalTaskInMillis(httpTaskFunction, 1000);
            return;
        }

        if (!isWifiConnected) {
            addPeriodicalTaskInMillis(httpTaskFunction, 1000);
            return;
        }

        isHttpQueriesQueueOccupied = true;

        HttpQueryQueueItem oldestQueryInQueue = httpQueriesQueue.front();

        switch (oldestQueryInQueue.type) {
            case BackendAppWeatherForecastAndAirPollutionQueries: {
                Serial.println("Processing new query from the queue: BackendAppWeatherForecastAndAirPollutionQueries");

                auto weatherItems = backendApp.fetchWeatherForecast();

                if (weatherItems.empty()) {
                    Serial.println("No weather data available");
                    addPeriodicalTaskInMillis(httpTaskFunction, 1000);
                    return;
                }

                WeatherItem weatherItem = weatherItems.front();
                
                backgroundApp.checkForWeatherWarning(weatherItems);

                auto airPollutionData = backendApp.fetchAirPollution();

                addWeatherLog(weatherItem.temperature, weatherItem.windSpeed, weatherItem.date, airPollutionData.pm25, airPollutionData.pm25Date, airPollutionData.pm10, airPollutionData.pm10Date);

                break;
            }

            case BackendAppSaveLogQuery: {
                Serial.println("Processing new query from the queue: BackendAppSaveLogQuery");
                backendApp.saveLogToApp(oldestQueryInQueue.backendAppLog);

                break;
            }       
        }

        httpQueriesQueue.erase(httpQueriesQueue.begin());
        isHttpQueriesQueueOccupied = false;

        disconnectWifi();
        // lastHttpRequestMillis = millis();
    }

    addPeriodicalTaskInMillis(httpTaskFunction, 1000);

    // No requests for 60 seconds, disconnect wifi
    // if (millis() - lastHttpRequestMillis > 60000) {
    //     disconnectWifi();
    // }
}

void wifiConnectionTaskFunction() {
    if (shouldDisplayFunctionTasksExecutionLogs) {
        Serial.println(">>> wifiConnectionTaskFunction executed");
    }

    if (isWifiConnected) {
        addPeriodicalTaskInMillis(wifiConnectionTaskFunction, 2000);
        return;
    }

    if (!shouldTryToConnectToWifi && !isWifiConnecting) {
        addPeriodicalTaskInMillis(wifiConnectionTaskFunction, 2000);
        return;
    }

    if (shouldTryToConnectToWifi) {
        shouldTryToConnectToWifi = false;
        isWifiConnecting = true;
    }

    Serial.println("Trying to connect to WiFi");

    auto wifiStatus = WiFi.waitForConnectResult();

    Serial.print("WiFi Status: ");
    Serial.println(wifiStatus);

    switch (wifiStatus) {
        case WL_CONNECTED: {
            isWifiConnected = true;
            isWifiConnecting = false;
            Serial.println("WiFi Connected");
            client->setInsecure();
            break;
        }

        case WL_CONNECT_FAILED:
        case WL_CONNECTION_LOST:
        case WL_DISCONNECTED: {
            isWifiConnected = false;
            isWifiConnecting = false;
            WiFi.disconnect();
            initWifi();
            break;
        }

        default: {
            isWifiConnected = false;
            isWifiConnecting = true;
            Serial.println("WiFi Connecting");
            break;
        }
    }

    addPeriodicalTaskInMillis(wifiConnectionTaskFunction, 1000);
}

void displayTaskFunction() {
    if (shouldDisplayFunctionTasksExecutionLogs) {
        Serial.println(">>> displayTaskFunction executed");
    }

    lcdWrapper.checkScroll();

    addPeriodicalTaskInMillis(displayTaskFunction, 1000);
}

void bleTaskFunction() {
    if (shouldDisplayFunctionTasksExecutionLogs) {
        Serial.println(">>> bleTaskFunction executed");
    }

    if (isBLEClientConnected) {
        bluetoothWrapper.checkQueue();

        addPeriodicalTaskInMillis(bleTaskFunction, 100);
        return;
    }

    addPeriodicalTaskInMillis(bleTaskFunction, 2000);
}

void batteryMeterTaskFunction() {
    if (shouldDisplayFunctionTasksExecutionLogs) {
        Serial.println(">>> batteryMeterTaskFunction executed");
    }

    if (batteryVoltageMetersAreActiveMemory.readValue() == 0) {
        addPeriodicalTaskInMillis(batteryMeterTaskFunction, 20000); // Once per 20 seconds
        return;
    }
    
    float batteryVoltageBox = batteryVoltageMeterBox.getVoltage();
    float batteryPercentageBox = batteryVoltageMeterBox.calculatePercentage(batteryVoltageBox);

    float batteryVoltageServos = batteryVoltageMeterServos.getVoltage();
    float batteryPercentageServos = batteryVoltageMeterServos.calculatePercentage(batteryVoltageServos);

    Serial.print("Battery Voltage Box: ");
    Serial.println(batteryVoltageMeterBox.getBatteryVoltageMessage());
    Serial.print("Battery Voltage Servos: ");
    Serial.println(batteryVoltageMeterServos.getBatteryVoltageMessage());

    if (batteryPercentageBox < BATTERY_VOLTAGE_MIN_PERCENTAGE || batteryPercentageServos < BATTERY_VOLTAGE_MIN_PERCENTAGE) {
        if (batteryPercentageBox < BATTERY_VOLTAGE_MIN_PERCENTAGE) {    
            Serial.println("Battery percentage is too low for Box");
        }

        if (batteryPercentageServos < BATTERY_VOLTAGE_MIN_PERCENTAGE) {
            Serial.println("Battery percentage is too low for Servos");
        }

        backgroundApp.addWarning(LOW_BATTERY);
    } else {
        backgroundApp.removeWarning(LOW_BATTERY);
    }

    addPeriodicalTaskInMillis(batteryMeterTaskFunction, 5000); // Once per 5 seconds
}

// Tasks
void checkPeriodicalTasksQueueTask(void *param) {
    while (true) {
        checkPeriodicalTasksQueue();

        vTaskDelay(100 / portTICK_PERIOD_MS);
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

unsigned long previousWindowOpeningCalculationMillis = 0;
void windowOpeningCalculationTask(void *param) {
    while (true) {
        // Checking if state has changed every second
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        if (!hasNTPAlreadyConfigured) {
            Serial.println("NTP not configured");
            continue;
        }

        unsigned long currentMillis = millis();
        uint16_t windowOpeningCalculationInterval = windowOpeningCalculationIntervalMemory.readValue(); // In seconds

        if (
            (
                AppMode == Auto && 
                currentMillis - previousWindowOpeningCalculationMillis > windowOpeningCalculationInterval * 1000
            ) || // Auto mode interval
            forceOpeningWindowCalculation == true // Forcing execution
        ) {
            Serial.println("Calculating window opening");

            float currentTemperature = noTemperatureMode ? optimalTemperatureMemory.readValue() : bme.readTemperature();
            auto [newWindowOpening, backendAppLog] = PIDController::calculateWindowOpening(currentTemperature);

            // Save to Backend
            Serial.println("Adding to queue: BackendAppSaveLogQuery");
            HttpQueryQueueItem queueItem = {
                type: BackendAppSaveLogQuery,
                backendAppLog: &backendAppLog
            };

            httpQueriesQueue.push_back(queueItem);

            uint8_t servoPullClosePosition = servoPullCloseWrapper.getCurrentPosition();
            uint8_t servoPullOpenPosition = servoPullOpenWrapper.getCurrentPosition();

            Serial.print("servoPullClosePosition: ");
            Serial.println(servoPullClosePosition);
            Serial.print("servoPullOpenPosition: ");
            Serial.println(servoPullOpenPosition);

            uint8_t servosAvaragePosition = (servoPullClosePosition + servoPullOpenPosition) / 2;

            Serial.print("servosAvaragePosition: ");
            Serial.println(servosAvaragePosition);

            if (abs(newWindowOpening - servosAvaragePosition) > 2) {
                boolean isWindowOpening = newWindowOpening > servosAvaragePosition; // Closing or Opening

                Serial.print("WindowCalculationTask: New Window Opening: ");
                Serial.println(newWindowOpening);

                if (isWindowOpening) {
                    servoPullCloseWrapper.setMovingSmoothlyTarget(newWindowOpening);
                    vTaskDelay(DELAY_DIFF_BETWEEN_SERVOS_MILISECONDS / portTICK_PERIOD_MS);
                    servoPullOpenWrapper.setMovingSmoothlyTarget(newWindowOpening);
                } else {
                    servoPullOpenWrapper.setMovingSmoothlyTarget(newWindowOpening);
                    vTaskDelay(DELAY_DIFF_BETWEEN_SERVOS_MILISECONDS / portTICK_PERIOD_MS);
                    servoPullCloseWrapper.setMovingSmoothlyTarget(newWindowOpening);
                }
            }

            previousWindowOpeningCalculationMillis = currentMillis; // Overriding also AUTO interval even if executed manually

            if (forceOpeningWindowCalculation == true) {
                forceOpeningWindowCalculation = false;
            }
        }
    }
}

void ntpTask(void *param) {
    while (true) {
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Once per second

        if (!isWifiConnected || hasNTPAlreadyConfigured) {
            continue;
        }

        if (!isNTPUnderConfiguration) {
            Serial.println("Starting NTP configuration");
            isNTPUnderConfiguration = true;
            configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER_URL); // Synchronize time
        }

        Serial.println("Trying to get current time");
        if (getCurrentTime() == "") {
            continue;
        } else {
            Serial.println("Current time obtained");
            hasNTPAlreadyConfigured = true; // It happens only once

            // Init first log (50 will be invalid value probably)
            float initialTemperature = noTemperatureMode ? optimalTemperatureMemory.readValue() : bme.readTemperature();
            addLog(initialTemperature, 50, 0);

            addPeriodicalTaskInMillis(httpTaskFunction, 100);
            
            vTaskDelete(NTPTask);
        }
    }
}

void checkMemoryTask(void *param) {
    while (true) {
        Serial.println("Free heap: " + String(esp_get_free_heap_size()) + " bytes");
        Serial.println("Minimum ever free heap: " + String(esp_get_minimum_free_heap_size()) + " bytes");

        UBaseType_t uxHighWaterMark;
        
        uxHighWaterMark = uxTaskGetStackHighWaterMark(CheckPeriodicalTasksQueue);
        Serial.printf("CheckPeriodicalTasksQueue minimum: %d / %d \n", uxHighWaterMark, CHECK_PERIODICAL_TASKS_QUEUE_TASK_STACK_SIZE);

        uxHighWaterMark = uxTaskGetStackHighWaterMark(CheckMemoryTask);
        Serial.printf("CheckMemoryTask minimum: %d / %d \n", uxHighWaterMark, CHECK_MEMORY_TASK_STACK_SIZE);

        uxHighWaterMark = uxTaskGetStackHighWaterMark(WindowOpeningCalculationTask);
        Serial.printf("WindowOpeningCalculationTask minimum: %d / %d \n", uxHighWaterMark, WINDOW_OPENING_CALCULATION_TASK_STACK_SIZE);

        // Optional
        if (!hasNTPAlreadyConfigured) {
            uxHighWaterMark = uxTaskGetStackHighWaterMark(NTPTask);
            Serial.printf("NTPTask minimum: %d / %d \n", uxHighWaterMark, NTP_TASK_STACK_SIZE);
        }

        uxHighWaterMark = uxTaskGetStackHighWaterMark(ServosSmoothMovementTask);
        Serial.printf("ServosSmoothMovementTask minimum: %d / %d \n", uxHighWaterMark, SERVOS_SMOOTH_MOVEMENT_TASK_STACK_SIZE);

        vTaskDelay(1000 / portTICK_PERIOD_MS); // Once per second
    }
}

void setup() {
    Wire.begin(LCD_SDA_GPIO, LCD_SCL_GPIO);
    Serial.begin(115200);

    batteryVoltageMeterBox.initialize();
    batteryVoltageMeterServos.initialize();

    initWifi();

    if (!EEPROM.begin(EEPROM_SIZE)) {
        Serial.println("EEPROM Error");
        return;
    }

    I2C_BME_280.begin(BME_280_SDA_GPIO, BME_280_SCL_GPIO, 100000); 

    if (!noTemperatureMode) {
        if (!bme.begin(BME280_ADDRESS, &I2C_BME_280)) {
            Serial.println("BME280 not working correctly");
            while (1);
        }

        Serial.println("BME280 initialized");
    }
    
    bluetoothWrapper.initialize();

    delay(100);

    navigation.initialize();
    ledWrapper.initialize();

    enterButton.initialize();
    exitButton.initialize();
    enterButton.attachButtonPressCallback(handleEnterButtonPress);
    exitButton.attachButtonPressCallback(handleExitButtonPress);

    servosPowerSupply.initialize();

    servoPullOpenWrapper.initialize(SERVO_PULL_OPEN_PWM_TIMER_INDEX);
    servoPullCloseWrapper.initialize(SERVO_PULL_CLOSE_PWM_TIMER_INDEX);

    // Tasks
    xTaskCreate(checkPeriodicalTasksQueueTask, "CheckPeriodicalTasksQueueTask", CHECK_PERIODICAL_TASKS_QUEUE_TASK_STACK_SIZE, NULL, 1, &CheckPeriodicalTasksQueue);
    xTaskCreate(servosSmoothMovementTask, "ServosSmoothMovementTask", SERVOS_SMOOTH_MOVEMENT_TASK_STACK_SIZE, NULL, 1, &ServosSmoothMovementTask);
    xTaskCreate(ntpTask, "NTPTask", NTP_TASK_STACK_SIZE, NULL, 1, &NTPTask);
    xTaskCreate(windowOpeningCalculationTask, "WindowOpeningCalculationTask", WINDOW_OPENING_CALCULATION_TASK_STACK_SIZE, NULL, 1, &WindowOpeningCalculationTask);

    // Optional
    // xTaskCreate(checkMemoryTask, "CheckMemoryTask", CHECK_MEMORY_TASK_STACK_SIZE, NULL, 1, &CheckMemoryTask);

    // Periodical Tasks
    addPeriodicalTaskInMillis(displayTaskFunction, 100);
    addPeriodicalTaskInMillis(warningsTaskFunction, 500);
    addPeriodicalTaskInMillis(weatherForecastAndAirPollutionTaskFunction, 700);
    addPeriodicalTaskInMillis(wifiConnectionTaskFunction, 900);
    addPeriodicalTaskInMillis(bleTaskFunction, 1100);
    addPeriodicalTaskInMillis(batteryMeterTaskFunction, 1300);

    lcdWrapper.initialize();
}

void loop() {
    // Navigation
    enterButton.checkButtonPress();
    exitButton.checkButtonPress();

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

    // Power Supply to Servos
    servosPowerSupply.checkTargetState();

    delay(20);
}