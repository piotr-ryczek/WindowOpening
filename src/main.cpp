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
#include <timeHelpers.h>

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
TaskHandle_t BluetoothCommandsTask;
TaskHandle_t HttpTask;

// Instances

LiquidCrystal_I2C lcd(0x27, 16, 2);
LcdWrapper lcdWrapper(&lcd);

HTTPClient httpClient;
WiFiClientSecure *client = new WiFiClientSecure;

Servo servoPullOpen;
Servo servoPullClose;
ServoWrapper servoPullOpenWrapper(SERVO_PULL_OPEN_GPIO, servoPullOpen, servoPullOpenCalibrationMinMemory, servoPullOpenCalibrationMaxMemory);
ServoWrapper servoPullCloseWrapper(SERVO_PULL_CLOSE_GPIO, servoPullClose, servoPullCloseCalibrationMinMemory, servoPullCloseCalibrationMaxMemory);

LedWrapper ledWrapper(LED_RED_PWM_TIMER_INDEX, LED_RED_GPIO, LED_GREEN_PWM_TIMER_INDEX, LED_GREEN_GPIO, LED_BLUE_PWM_TIMER_INDEX, LED_BLUE_GPIO);

Navigation navigation(POTENTIOMETER_GPIO, servoPullOpenWrapper, servoPullCloseWrapper, ledWrapper, &AppMode, &lcdWrapper);
ButtonHandler enterButton(ENTER_BUTTON_GPIO);
ButtonHandler exitButton(EXIT_BUTTON_GPIO);

BackgroundApp backgroundApp(ledWrapper, lcdWrapper);
BackendApp backendApp(&httpClient, &backgroundApp);

Adafruit_BME280 bme;

BluetoothWrapper bluetoothWrapper(&bme, &backgroundApp);

enum HttpQueryTypeEnum { BackendAppWeatherForecastAndAirPollutionQueries, BackendAppSaveLogQuery };

struct HttpQueryQueueItem {
    HttpQueryTypeEnum type;
    BackendAppLog* backendAppLog;
};

bool isHttpQueriesQueueOccupied = false;

vector<HttpQueryQueueItem> httpQueriesQueue;

void initWifi() {
    // WiFi.mode(WIFI_STA);
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


unsigned long previousWindowOpeningCalculationMillis = 0;
void windowOpeningCalculationTask(void *param) {
    while (true) {
        unsigned long currentMillis = millis();
        uint16_t windowOpeningCalculationInterval = windowOpeningCalculationIntervalMemory.readValue(); // In seconds

        if (
            AppMode == Auto && currentMillis - previousWindowOpeningCalculationMillis > windowOpeningCalculationInterval * 1000 // Auto mode interval
            || forceOpeningWindowCalculation == true // Forcing execution
        ) {
            float currentTemperature = bme.readTemperature();
            auto [newWindowOpening, backendAppLog] = PIDController::calculateWindowOpening(currentTemperature);

            // Save to Backend
            Serial.println("Adding to queue: BackendAppSaveLogQuery");
            HttpQueryQueueItem queueItem = {
                type: BackendAppSaveLogQuery,
                backendAppLog: &backendAppLog
            };

            httpQueriesQueue.push_back(queueItem);

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

            previousWindowOpeningCalculationMillis = currentMillis; // Overriding also AUTO interval even if executed manually

            if (forceOpeningWindowCalculation == true) {
                forceOpeningWindowCalculation = false;
            }
        }

        // Checking if state has changed every second
        vTaskDelay(1000 / portTICK_PERIOD_MS);
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

void httpTask(void *param) {
    while (true) {
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Once per second seconds

        // TODO: Possibly to remove - should be connection existence checked?
        // if (bluetoothWrapper.hasClient()) {
        //     continue;
        // }

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

            HttpQueryQueueItem olderQueryInQueue = httpQueriesQueue.front();

            switch (olderQueryInQueue.type) {
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
                    backendApp.saveLogToApp(olderQueryInQueue.backendAppLog);

                    break;
                }       
            }

            httpQueriesQueue.erase(httpQueriesQueue.begin());
            isHttpQueriesQueueOccupied = false;

            disconnectWifi();
        }
    }
}

void wifiConnectionTask(void *param) {
    while (true) {
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Once per second

        if (!shouldTryToConnectToWifi && !isWifiConnecting && !isWifiConnected) {
            continue;
        } 

        if (isWifiConnected) {
            continue;
        }
        
        if (shouldTryToConnectToWifi) {
            shouldTryToConnectToWifi = false;
            isWifiConnecting = true;
        }

        auto wifiStatus = WiFi.status();
        
        if (wifiStatus == WL_CONNECTED) {
            isWifiConnected = true;
            isWifiConnecting = false;
            Serial.println("WiFi Connected");
            client->setInsecure();

            if (!hasNTPAlreadyConfigured) {
                configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER_URL); // Synchronize time

                if (getCurrentTime() == "") {
                    continue;
                } else {
                    hasNTPAlreadyConfigured = true; // It happens only once
                    Serial.println("Current time obtained");
                    xTaskCreate(httpTask, "HttpTask", 10240, NULL, 5, &HttpTask);
                }

                vTaskDelay(3000 / portTICK_PERIOD_MS); // Delay for 3s to give a time to retrieve current time (for first time)
            }
        } else {
            isWifiConnected = false;
            Serial.println("WiFi Connecting");
            Serial.print("WiFi Status:");
            Serial.println(wifiStatus);
        }
    }
}

void displayTask(void *param) {
    while (true) {
        lcdWrapper.checkScroll();

        vTaskDelay(1000 / portTICK_PERIOD_MS); // Once per second
    }
}

void setup() {
    Wire.begin(LCD_SDA_GPIO, LCD_SCL_GPIO);
    Serial.begin(115200);

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
    bluetoothWrapper.init();

    delay(100);

    // Init first log (50 will be invalid value probably)
    float initialTemperature = bme.readTemperature();
    addLog(initialTemperature, 50, 0);

    ledWrapper.initialize();

    enterButton.initialize();
    exitButton.initialize();
    enterButton.attachButtonPressCallback(handleEnterButtonPress);
    exitButton.attachButtonPressCallback(handleExitButtonPress);

    servoPullOpenWrapper.initialize(SERVO_PULL_OPEN_PWM_TIMER_INDEX);
    servoPullCloseWrapper.initialize(SERVO_PULL_CLOSE_PWM_TIMER_INDEX);
    
    xTaskCreate(warningsTask, "WarningsTask", 1536, NULL, 1, &WarningsTask);
    xTaskCreate(servosSmoothMovementTask, "ServosSmoothMovementTask", 1024, NULL, 1, &ServosSmoothMovementTask);
    xTaskCreate(displayTask, "displayTask", 1536, NULL, 1, &DisplayTask);
    xTaskCreate(weatherForecastAndAirPollutionTask, "weatherForecastAndAirPollutionTask", 768, NULL, 1, &WeatherForecastAndAirPollutionTask);
    xTaskCreate(navigationTask, "NavigationTask", 2048, NULL, 1, &NavigationTask);
    xTaskCreate(wifiConnectionTask, "WifiConnectionTask", 2048, NULL, 1, &WifiConnectionTask);

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