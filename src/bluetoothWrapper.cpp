#include <Arduino.h>
#include <unordered_map>
#include <string>
#include <sstream>
#include <algorithm>
#include <vector>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#include <bluetoothWrapper.h>
#include <secrets.h>
#include <config.h>
#include <logs.h>
#include <memoryData.h>

using namespace std;

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
  "FORCE_OPENING_WINDOW_CALCULATION"
};

BluetoothWrapper::BluetoothWrapper(Adafruit_BME280* bme, BackgroundApp* backgroundApp): bme(bme), backgroundApp(backgroundApp) {}

class WindowOpeningBLEServerCallbacks : public BLEServerCallbacks {
  public:
    void onConnect(BLEServer* pServer) override {
        Serial.println("Client connected");
    }

    void onDisconnect(BLEServer* pServer) override {
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
        std::string value = pCharacteristic->getValue();
        Serial.print("BLE Received: ");
        Serial.println(value.c_str());

        value.erase(std::remove(value.begin(), value.end(), '\n'), value.end());
        value.erase(std::remove(value.begin(), value.end(), '\r'), value.end());

        String response = bluetoothWrapper->handleCommand(new String(value.c_str()));

        pCharacteristic->setValue(response.c_str());
    }
};

void BluetoothWrapper::init() {
  BLEDevice::init(BLE_NAME);
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(BLE_SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(BLE_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);

  pServer->setCallbacks(new WindowOpeningBLEServerCallbacks());
  pCharacteristic->setCallbacks(new WindowOpeningBLECharacteristicCallbacks(this));
  pService->start();

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();

  settingsMemory["OPTIMAL_TEMPERATURE"] = &optimalTemperatureMemory;
  settingsMemory["P_TERM_POSITIVE"] = &pTermPositive;
  settingsMemory["P_TERM_NEGATIVE"] = &pTermNegative;
  settingsMemory["D_TERM_POSITIVE"] = &dTermPositive;
  settingsMemory["D_TERM_NEGATIVE"] = &dTermNegative;
  settingsMemory["O_TERM_POSITIVE"] = &oTermPositive;
  settingsMemory["O_TERM_NEGATIVE"] = &oTermNegative;
  settingsMemory["I_TERM"] = &iTerm;
  settingsMemory["CHANGE_DIFF_THRESHOLD"] = &changeDiffThresholdMemory;
  settingsMemory["WINDOW_OPENING_CALCULATION_INTERVAL"] = &windowOpeningCalculationIntervalMemory;
  settingsMemory["OPENING_TERM_POSITIVE_TEMPERATURE_INCREASE"] = &openingTermPositiveTemperatureIncrease;

  Serial.println("Bluetooth initialized. Ready for pairing");
}

String BluetoothWrapper::handleCommand(String* message) {
  Serial.print("Bluetooth data received: ");
  Serial.println(message->c_str());
  
  vector<string> parts = this->splitString(message);

  if (parts.empty()) {
    Serial.println("Not processing bluetooth command: zero parts");
    return "Invalid Command: Zero Parts";
}

  if (parts.size() < 1 && parts.size() > 3) {
    Serial.println("Not processing bluetooth command: less than 1 or more than 3 parts");
    return "Invalid Command: Less than 1 or more than 3 parts";
  }

  string commandType = trim(parts.at(0));

  MemoryValue* memoryData = nullptr;

  if (commandType == "SET" || commandType == "GET") {
    string property = trim(parts.at(1));
    
    auto it = settingsMemory.find(property);

    if (it == settingsMemory.end()) {
      Serial.println("Invalid property");
      return "Invalid Property";
    }

    memoryData = it->second;
  }

  if (commandType == "SET") {
    return handleSetCommand(memoryData, stoi(trim(parts.at(2))));
  } else if (commandType == "GET") {
    return handleGetCommand(memoryData);
  } else if (commandType == "GET_LOGS") {
    return handleGetLogsCommand();
  } else if (commandType == "GET_TEMPERATURE") {
    return handleGetTemperatureCommand();
  } else if (commandType == "SET_APP_MODE_AUTO") {
    return handleSetAppModeAutoCommand();
  } else if (commandType == "SET_APP_MODE_MANUAL") {
    return handleSetAppModeManualCommand();
  } else if (commandType == "CLEAR_WARNINGS") {
    return handleClearWarningsCommand();
  } else if (commandType == "GET_LAST_WEATHER_LOG") {
    return handleGetLastWeatherLogCommand();
  } else if (commandType == "FORCE_OPENING_WINDOW_CALCULATION") {
    return handleForceOpeningWindowCalculationCommand();
  } else {
    return handleInvalidCommand();
  }
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

String BluetoothWrapper::handleGetLogsCommand() {
  vector<Log> lastLogs = getLastLogs(10);

  for (const auto& log: lastLogs) {
    Serial.print("Temperature: ");
    Serial.println(log.temperature);
    Serial.print("WindowOpening: ");
    Serial.println(log.windowOpening);
    Serial.print("DeltaTemporaryWindowOpening: ");
    Serial.println(log.deltaTemporaryWindowOpening);
    Serial.println("----------------");
  }

  return "TODO";
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

  return "TODO";
}

String BluetoothWrapper::handleForceOpeningWindowCalculationCommand() {
  forceOpeningWindowCalculation = true;

  Serial.println("Opening Window Calculation Forced");
  return "Opening Window Calculation Forced";
}

String BluetoothWrapper::handleInvalidCommand() {
  Serial.println("Invalid command");
  return "Invalid command";
}