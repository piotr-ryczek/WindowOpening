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
#include <valuesJitterFilter.h>

/**
 * How to simulate calculations:
 * AppModeEnum AppMode = Manual; -> Auto
 * float currentTemperature = bme.readTemperature(); -> hardcode temperature
 */

/**
 * Further next:
 * - WebApp to aggregate data
 */

#define EEPROM_SIZE 768
#define BME280_ADDRESS 0x76

using namespace std;

TwoWire I2C_BME_280 = TwoWire(1);

TaskHandle_t NavigationTask;
TaskHandle_t WarningsTask;
TaskHandle_t ServosSmoothMovementTask;
TaskHandle_t WifiConnectionTask;
TaskHandle_t WeatherForecastAndAirPollutionTask;
TaskHandle_t WindowOpeningCalculationTask;
TaskHandle_t DisplayTask;
TaskHandle_t HttpTask;
TaskHandle_t NTPTask;
TaskHandle_t BLETask;
TaskHandle_t BatteryMeterTask;
// Instances

ValuesJitterFilter valuesJitterFilter;

LiquidCrystal_I2C lcd(0x27, 16, 2);
LcdWrapper lcdWrapper(&lcd);

HTTPClient httpClient;
WiFiClientSecure *client = new WiFiClientSecure;

BatteryVoltageMeter batteryVoltageMeterBox(BATTERY_VOLTAGE_BOX_METER_GPIO, BATTERY_VOLTAGE_0_REFERENCE, BATTERY_VOLTAGE_100_REFERENCE, lastReadBatteryVoltageBox);
BatteryVoltageMeter batteryVoltageMeterServos(BATTERY_VOLTAGE_SERVOS_METER_GPIO, BATTERY_VOLTAGE_0_REFERENCE, BATTERY_VOLTAGE_100_REFERENCE, lastReadBatteryVoltageServos);

Servo servoPullOpen;
Servo servoPullClose;
ServoWrapper servoPullOpenWrapper(SERVO_PULL_OPEN_GPIO, servoPullOpen, servoPullOpenCalibrationMinMemory, servoPullOpenCalibrationMaxMemory);
ServoWrapper servoPullCloseWrapper(SERVO_PULL_CLOSE_GPIO, servoPullClose, servoPullCloseCalibrationMinMemory, servoPullCloseCalibrationMaxMemory);

LedWrapper ledWrapper(LED_RED_PWM_TIMER_INDEX, LED_RED_GPIO, LED_GREEN_PWM_TIMER_INDEX, LED_GREEN_GPIO, LED_BLUE_PWM_TIMER_INDEX, LED_BLUE_GPIO);

Navigation navigation(POTENTIOMETER_GPIO, true, servoPullOpenWrapper, servoPullCloseWrapper, ledWrapper, &AppMode, &lcdWrapper, &batteryVoltageMeterBox, &batteryVoltageMeterServos, &valuesJitterFilter);
ButtonHandler enterButton(ENTER_BUTTON_GPIO);
ButtonHandler exitButton(EXIT_BUTTON_GPIO);

BackgroundApp backgroundApp(ledWrapper, lcdWrapper);
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

            float currentTemperature = bme.readTemperature();
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
                    vTaskDelay(DELAY_DIFF_BETWEEN_SERVOS_MILISECONDS);
                    servoPullOpenWrapper.setMovingSmoothlyTarget(newWindowOpening);
                } else {
                    servoPullOpenWrapper.setMovingSmoothlyTarget(newWindowOpening);
                    vTaskDelay(DELAY_DIFF_BETWEEN_SERVOS_MILISECONDS);
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

void weatherForecastAndAirPollutionTask(void *param) {
    while (true) {
        Serial.println("Adding to queue: BackendAppWeatherForecastAndAirPollutionQueries");
        HttpQueryQueueItem weatherForecastAndAirPollutionQueueItem = {
            type: BackendAppWeatherForecastAndAirPollutionQueries,
            backendAppLog: nullptr
        };

        httpQueriesQueue.push_back(weatherForecastAndAirPollutionQueueItem);

        vTaskDelay(1000 * 60 * 60 / portTICK_PERIOD_MS); // Once per hour
    }
}

// unsigned long lastHttpRequestMillis = 0;
void httpTask(void *param) {
    while (true) {
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Once per second seconds

        if (!httpQueriesQueue.empty()) {
            if (isHttpQueriesQueueOccupied) {
                continue;
            }

            if (!isWifiConnected && !isWifiConnecting) {
                initWifi();
                continue;
            }

            if (!isWifiConnected) {
                continue;
            }

            isHttpQueriesQueueOccupied = true;

            HttpQueryQueueItem oldestQueryInQueue = httpQueriesQueue.front();

            switch (oldestQueryInQueue.type) {
                case BackendAppWeatherForecastAndAirPollutionQueries: {
                    Serial.println("Processing new query from the queue: BackendAppWeatherForecastAndAirPollutionQueries");

                    auto weatherItems = backendApp.fetchWeatherForecast();

                    if (weatherItems.empty()) {
                        Serial.println("No weather data available");
                        vTaskDelay(1000 / portTICK_PERIOD_MS); // Once per second seconds
                        continue;
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

        // No requests for 60 seconds, disconnect wifi
        // if (millis() - lastHttpRequestMillis > 60000) {
        //     disconnectWifi();
        // }
    }
}

void wifiConnectionTask(void *param) {
    while (true) {
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Once per second

        if (isWifiConnected) {
            continue;
        }

        if (!shouldTryToConnectToWifi && !isWifiConnecting) {
            continue;
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
            float initialTemperature = bme.readTemperature();
            addLog(initialTemperature, 50, 0);

            xTaskCreate(httpTask, "HttpTask", 10240, NULL, 5, &HttpTask);
            vTaskDelete(NTPTask);
        }
    }
}

void displayTask(void *param) {
    while (true) {
        lcdWrapper.checkScroll();

        vTaskDelay(1000 / portTICK_PERIOD_MS); // Once per second
    }
}

void bleTask(void *param) {
    while (true) {
        if (isBLEClientConnected) {
            bluetoothWrapper.checkQueue();
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void batteryMeterTask(void *param) {
    while (true) {
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

        vTaskDelay(5000 / portTICK_PERIOD_MS); // Once per 5 seconds
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

    if (!bme.begin(BME280_ADDRESS, &I2C_BME_280)) {
        Serial.println("BME280 not working correctly");
        while (1);
    }

    Serial.println("BME280 initialized");
    bluetoothWrapper.initialize();

    delay(100);

    navigation.initialize();
    ledWrapper.initialize();

    enterButton.initialize();
    exitButton.initialize();
    enterButton.attachButtonPressCallback(handleEnterButtonPress);
    exitButton.attachButtonPressCallback(handleExitButtonPress);

    servoPullOpenWrapper.initialize(SERVO_PULL_OPEN_PWM_TIMER_INDEX);
    servoPullCloseWrapper.initialize(SERVO_PULL_CLOSE_PWM_TIMER_INDEX);
    
    xTaskCreate(warningsTask, "WarningsTask", 1536, NULL, 1, &WarningsTask);
    xTaskCreate(servosSmoothMovementTask, "ServosSmoothMovementTask", 1536, NULL, 1, &ServosSmoothMovementTask);
    xTaskCreate(displayTask, "displayTask", 1536, NULL, 1, &DisplayTask);
    xTaskCreate(weatherForecastAndAirPollutionTask, "weatherForecastAndAirPollutionTask", 1536, NULL, 1, &WeatherForecastAndAirPollutionTask);
    xTaskCreate(navigationTask, "NavigationTask", 2048, NULL, 1, &NavigationTask);
    xTaskCreate(wifiConnectionTask, "WifiConnectionTask", 3072, NULL, 1, &WifiConnectionTask);
    xTaskCreate(ntpTask, "NTPTask", 3072, NULL, 1, &NTPTask);
    xTaskCreate(windowOpeningCalculationTask, "WindowOpeningCalculationTask", 4096 , NULL, 1, &WindowOpeningCalculationTask);
    xTaskCreate(bleTask, "BLETask", 3072 , NULL, 1, &BLETask);
    xTaskCreate(batteryMeterTask, "BatteryMeterTask", 1536 , NULL, 1, &BatteryMeterTask);

    lcdWrapper.initialize();
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