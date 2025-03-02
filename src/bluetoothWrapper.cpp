#include <Arduino.h>
#include <unordered_map>
#include <string>
#include <sstream>
#include <algorithm>
#include <vector>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <ArduinoJson.h>
#include <BLE2902.h>

#include <bluetoothWrapper.h>
#include <secrets.h>
#include <config.h>
#include <logs.h>
#include <memoryData.h>
#include <helpers.h>

using namespace std;

vector<String> BLEQueue;
unordered_map<string, MemoryValue*> settingsMemory;
vector<string> commandTypes = {
  "GET", // GET PROPERTY_NAME
  "SET", // SET PROPERTY_NAME VALUE
  "GET_LOGS", 
  "GET_TEMPERATURE", 
  "SET_APP_MODE_AUTO", 
  "SET_APP_MODE_MANUAL", 
  "GET_LAST_WEATHER_LOG",
  "CLEAR_WARNINGS",
  "FORCE_OPENING_WINDOW_CALCULATION",
  "MOVE_BOTH_SERVOS_SMOOTHLY_TO",
  "GET_BATTERY_VOLTAGE_BOX",
  "GET_BATTERY_VOLTAGE_SERVOS",
};

BluetoothWrapper::BluetoothWrapper(Adafruit_BME280* bme, BackgroundApp* backgroundApp, ServoWrapper* servoPullOpen, ServoWrapper* servoPullClose, BatteryVoltageMeter* batteryVoltageMeterBox, BatteryVoltageMeter* batteryVoltageMeterServos): bme(bme), backgroundApp(backgroundApp), servoPullOpen(servoPullOpen), servoPullClose(servoPullClose), batteryVoltageMeterBox(batteryVoltageMeterBox), batteryVoltageMeterServos(batteryVoltageMeterServos) {}

class WindowOpeningBLEServerCallbacks : public BLEServerCallbacks {
  public:
    void onConnect(BLEServer* pServer) override {
        isBLEClientConnected = true;
        Serial.println("Client connected");
    }

    void onDisconnect(BLEServer* pServer) override {
      isBLEClientConnected = false;
      pServer->startAdvertising();
      Serial.println("Client disconnected");
    }
};

class WindowOpeningBLECharacteristicCallbacks : public BLECharacteristicCallbacks {
  private:
    BluetoothWrapper* bluetoothWrapper;

  public:
    WindowOpeningBLECharacteristicCallbacks(BluetoothWrapper* wrapper) : bluetoothWrapper(wrapper) {}

    WindowOpeningBLECharacteristicCallbacks() {}

    void onWrite(BLECharacteristic* pCharacteristic) override {
      if (!bluetoothWrapper) {
          Serial.println("BluetoothWrapper is not initialized");
          return;
      }

      std::string value = pCharacteristic->getValue();
      Serial.print("BLE Received: ");
      Serial.println(value.c_str());

      value.erase(std::remove(value.begin(), value.end(), '\n'), value.end());
      value.erase(std::remove(value.begin(), value.end(), '\r'), value.end());

      String valueString = value.c_str();
      auto [response, commandType] = bluetoothWrapper->handleCommand(&valueString);

      for (const auto& item : response) {
        StaticJsonDocument<400> jsonDoc;
        JsonObject jsonObject = jsonDoc.createNestedObject();
        jsonObject["commandType"] = commandType;
        jsonObject["data"] = item;

        String jsonString;
        serializeJson(jsonDoc, jsonString);

        // noInterrupts();
        BLEQueue.push_back(jsonString);
        // interrupts();
      }
    }
};

void BluetoothWrapper::initialize() {
  BLEDevice::init(BLE_NAME);
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(BLE_SERVICE_UUID);
  this->pCharacteristic = pService->createCharacteristic(BLE_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_INDICATE | BLECharacteristic::PROPERTY_NOTIFY);

  this->pCharacteristic->addDescriptor(new BLE2902());

  pServer->setCallbacks(new WindowOpeningBLEServerCallbacks());
  this->pCharacteristic->setCallbacks(new WindowOpeningBLECharacteristicCallbacks(this));
  pService->start();

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();

  settingsMemory["OPTIMAL_TEMPERATURE"] = &optimalTemperatureMemory;
  settingsMemory["P_TERM_POSITIVE"] = &pTermPositiveMemory;
  settingsMemory["P_TERM_NEGATIVE"] = &pTermNegativeMemory;
  settingsMemory["D_TERM_POSITIVE"] = &dTermPositiveMemory;
  settingsMemory["D_TERM_NEGATIVE"] = &dTermNegativeMemory;
  settingsMemory["O_TERM_POSITIVE"] = &oTermPositiveMemory;
  settingsMemory["O_TERM_NEGATIVE"] = &oTermNegativeMemory;
  settingsMemory["I_TERM"] = &iTermMemory;
  settingsMemory["CHANGE_DIFF_THRESHOLD"] = &changeDiffThresholdMemory;
  settingsMemory["WINDOW_OPENING_CALCULATION_INTERVAL"] = &windowOpeningCalculationIntervalMemory;
  settingsMemory["OPENING_TERM_POSITIVE_TEMPERATURE_INCREASE"] = &openingTermPositiveTemperatureIncreaseMemory;

  Serial.println("Bluetooth initialized. Ready for pairing");
}

void BluetoothWrapper::checkQueue() {
  if (BLEQueue.empty()) return;

  String firstMessage = BLEQueue.front();
  const char* firstMessageChar = firstMessage.c_str();

  Serial.println("Notify BLE with: ");
  Serial.println(firstMessageChar);

  this->pCharacteristic->setValue(firstMessageChar);
  this->pCharacteristic->notify();
  BLEQueue.erase(BLEQueue.begin());
}

tuple<vector<String>, String> BluetoothWrapper::handleCommand(String* message) {
  Serial.print("Bluetooth data received: ");
  Serial.println(message->c_str());
  
  vector<String> response;
  vector<string> parts = this->splitString(message);

  if (parts.empty()) {
    Serial.println("Not processing bluetooth command: zero parts");
    response.push_back("Invalid Command: Zero Parts");
}

  if (parts.size() < 1 || parts.size() > 3) {
    Serial.println("Not processing bluetooth command: less than 1 or more than 3 parts");
    response.push_back("Invalid Command: Less than 1 or more than 3 parts");
  }

  string commandType = trim(parts.at(0));

  MemoryValue* memoryData = nullptr;
  uint8_t newServosPosition;

  if (commandType == "SET" || commandType == "GET") {
    string property = trim(parts.at(1));
    
    auto it = settingsMemory.find(property);

    if (it == settingsMemory.end()) {
      Serial.println("Invalid property");
      response.push_back("Invalid Property");
    }

    memoryData = it->second;
  }

  if (commandType == "MOVE_BOTH_SERVOS_SMOOTHLY_TO") {
    try {
      newServosPosition = convertStringToUint8t(trim(parts.at(1)));
    } catch (std::invalid_argument& error) {
      Serial.println(error.what());
      response.push_back(error.what());
    }
  }

  // If any error exit execution
  if (response.size() > 0) {
    return make_tuple(response, String('ERROR'));
  }

  if (commandType == "SET") {
    response.push_back(handleSetCommand(memoryData, stoi(trim(parts.at(2)))));
  } else if (commandType == "GET") {
    response.push_back(handleGetCommand(memoryData));
  } else if (commandType == "GET_LOGS") {
    // Notification
    response = handleGetLogsCommand(); // Multiple

    return make_tuple(response, commandType.c_str());
  } else if (commandType == "GET_TEMPERATURE") {
    response.push_back(handleGetTemperatureCommand());
  } else if (commandType == "SET_APP_MODE_AUTO") {
    response.push_back(handleSetAppModeAutoCommand());
  } else if (commandType == "SET_APP_MODE_MANUAL") {
    response.push_back(handleSetAppModeManualCommand());
  } else if (commandType == "CLEAR_WARNINGS") {
    response.push_back(handleClearWarningsCommand());
  } else if (commandType == "GET_LAST_WEATHER_LOG") {
    response.push_back(handleGetLastWeatherLogCommand());
  } else if (commandType == "FORCE_OPENING_WINDOW_CALCULATION") {
    response.push_back(handleForceOpeningWindowCalculationCommand());
  } else if (commandType == "MOVE_BOTH_SERVOS_SMOOTHLY_TO") {
    response.push_back(handleMoveBothServosSmoothlyTo(newServosPosition));
  } else if (commandType == "GET_BATTERY_VOLTAGE_BOX") {
    response.push_back(handleGetBatteryVoltageCommand(batteryVoltageMeterBox));
  } else if (commandType == "GET_BATTERY_VOLTAGE_SERVOS") {
    response.push_back(handleGetBatteryVoltageCommand(batteryVoltageMeterServos));
  } else {
    response.push_back(handleInvalidCommand());
  }

  return make_tuple(response, commandType.c_str());
}

vector<string> BluetoothWrapper::splitString(const String* command) {
  vector<string> parts;
  stringstream ss(command->c_str());
  string token;

  while (getline(ss, token, ' ')) {
    parts.push_back(token);
  }

  if (parts.size() < 1 || parts.size() > 3) {
    Serial.println("Invalid command: parts less than 1 or more than 3");

    return vector<string>();
  }

  string commandType = trim(parts.at(0));

  auto it = find(commandTypes.begin(), commandTypes.end(), commandType);

  if (it == commandTypes.end()) {
    Serial.println("Invalid command: invalid command type");

    return vector<string>();
  }

  if (
    (commandType == "GET_LOGS" || commandType == "GET_TEMPERATURE" || commandType == "SET_APP_MODE_AUTO" || commandType == "SET_APP_MODE_MANUAL" || commandType == "GET_LAST_WEATHER_LOG")
    && parts.size() != 1
  ) {
    Serial.println("Invalid command: Command expects to be the only one part");

    return vector<string>();
  }

  if (commandType == "SET" && parts.size() != 3) {
    Serial.println("Invalid command: SET parts different from 3");

    return vector<string>();
  }

  if (commandType == "GET" && parts.size() != 2) {
    Serial.println("Invalid command: GET parts different from 2");

    return vector<string>();
  }

  return parts;
}

string BluetoothWrapper::trim(const string& str) {
  size_t first = str.find_first_not_of(" \t\n\r");
  size_t last = str.find_last_not_of(" \t\n\r");
  return (first == string::npos || last == string::npos) ? "" : str.substr(first, (last - first + 1));
}

String BluetoothWrapper::handleSetCommand(MemoryValue* memoryData, int value) {
  memoryData->setValue(value);

  Serial.println("Value saved:");
  Serial.println(value);

  return "Value saved";
}

String BluetoothWrapper::handleGetCommand(MemoryValue* memoryData) {
  int memoryValue = memoryData->readValue();

  Serial.println("Value read:");
  Serial.println(memoryValue);

  return String(memoryValue);
}

vector<String> BluetoothWrapper::handleGetLogsCommand() {
  vector<Log> lastLogs = getLastLogs(10);
  vector<String> response;

  for (const auto& log: lastLogs) {
    StaticJsonDocument<300> jsonDoc;
    JsonArray jsonLogs = jsonDoc.createNestedArray("logs");

    Serial.print("Date: ");
    Serial.println(log.date);
    Serial.print("Temperature: ");
    Serial.println(log.temperature);
    Serial.print("WindowOpening: ");
    Serial.println(log.windowOpening);
    Serial.print("DeltaTemporaryWindowOpening: ");
    Serial.println(log.deltaTemporaryWindowOpening);
    Serial.println("----------------");

    JsonObject jsonLogObject = jsonLogs.createNestedObject();
    jsonLogObject["date"] = log.date;
    jsonLogObject["temperature"] = log.temperature;
    jsonLogObject["windowOpening"] = log.windowOpening;
    jsonLogObject["deltaTemporaryWindowOpening"] = log.deltaTemporaryWindowOpening;

    String jsonString;
    serializeJson(jsonDoc, jsonString);

    response.push_back(jsonString);
  }

  return response;
}

String BluetoothWrapper::handleGetTemperatureCommand() {
  float temperature = bme->readTemperature();

  Serial.println(temperature);
  return String(temperature);
}

String BluetoothWrapper::handleSetAppModeAutoCommand() {
  AppMode = Auto;

  Serial.println("AppMode changed to Auto");
  return "AppMode changed to Auto";
}

String BluetoothWrapper::handleSetAppModeManualCommand() {
  AppMode = Manual;

  Serial.println("AppMode changed to Manual");
  return "AppMode changed to Manual";
}

String BluetoothWrapper::handleClearWarningsCommand() {
  backgroundApp->clearWarnings();

  Serial.println("Warnings cleared");
  return "Warnings cleared";
}

String BluetoothWrapper::handleGetLastWeatherLogCommand() {
  WeatherLog* weatherLog = getLastWeatherLogNotTooOld(10);

  if (weatherLog == nullptr) {
    Serial.println("No weather logs available");

    return "No weather logs available";
  }

  StaticJsonDocument<200> jsonDoc;
  JsonObject jsonLogObject = jsonDoc.createNestedObject();

  jsonLogObject["forecastDate"] = weatherLog->forecastDate;
  jsonLogObject["outsideTemperature"] = weatherLog->outsideTemperature;
  jsonLogObject["windSpeed"] = weatherLog->windSpeed;
  jsonLogObject["pm10"] = weatherLog->pm10;
  jsonLogObject["pm10Date"] = weatherLog->pm10Date;
  jsonLogObject["pm25"] = weatherLog->pm25;
  jsonLogObject["pm25Date"] = weatherLog->pm25Date;

  Serial.print("forecastDate: ");
  Serial.println(weatherLog->forecastDate);
  Serial.print("outsideTemperature: ");
  Serial.println(weatherLog->outsideTemperature);
  Serial.print("windSpeed: ");
  Serial.println(weatherLog->windSpeed);
  Serial.print("pm10: ");
  Serial.println(weatherLog->pm10);
  Serial.print("pm10Date: ");
  Serial.println(weatherLog->pm10Date);
  Serial.print("pm25: ");
  Serial.println(weatherLog->pm25);
  Serial.print("pm10Date: ");
  Serial.println(weatherLog->pm10Date);
  
  String jsonString;
  serializeJson(jsonDoc, jsonString);

  return jsonString;
}

String BluetoothWrapper::handleForceOpeningWindowCalculationCommand() {
  forceOpeningWindowCalculation = true;

  Serial.println("Opening Window Calculation Forced");
  return "Opening Window Calculation Forced";
}

String BluetoothWrapper::handleMoveBothServosSmoothlyTo(uint8_t newPosition) {
  servoPullOpen->setMovingSmoothlyTarget(newPosition);
  servoPullClose->setMovingSmoothlyTarget(newPosition);

  Serial.print("New target set up: ");
  Serial.println(newPosition);
  return "New target set up to: " + String(newPosition);
}

String BluetoothWrapper::handleGetBatteryVoltageCommand(BatteryVoltageMeter* batteryVoltageMeter) {
  float batteryVoltage = batteryVoltageMeter->getVoltage();
  float batteryPercentage = batteryVoltageMeter->calculatePercentage(batteryVoltage);

  return batteryVoltageMeter->getBatteryVoltageMessage();
}

String BluetoothWrapper::handleInvalidCommand() {
  Serial.println("Invalid command");
  return "Invalid command";
}