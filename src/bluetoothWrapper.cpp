#include <Arduino.h>
#include <unordered_map>
#include <string>
#include <sstream>
#include <algorithm>
#include <vector>

#include <bluetoothWrapper.h>
#include <secrets.h>
#include <config.h>
#include <logs.h>
#include <memoryData.h>

using namespace std;

unordered_map<string, MemoryValue*> settingsMemory;
vector<string> commandTypes = {"GET", "SET", "GET_LOGS", "GET_TEMPERATURE", "SET_APP_MODE_AUTO", "SET_APP_MODE_MANUAL", "CLEAR_WARNINGS"};

BluetoothWrapper::BluetoothWrapper(BluetoothSerial* serialBT, Adafruit_BME280* bme, BackgroundApp* backgroundApp): serialBT(serialBT), bme(bme), backgroundApp(backgroundApp) {}

void BluetoothWrapper::init() {
  if (!serialBT) {
    Serial.println("BluetoothSerial not initialized");
    return;
  }

  if (!serialBT->begin(BLUETOOTH_NAME)) {
    Serial.println("Bluetooth initialization failed");
    while (1);
  }

  // if (!serialBT->setPin(BLUETOOTH_PIN)) {
  //   Serial.println("Bluetooth could not set password");
  //   while (1);
  // }

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

void BluetoothWrapper::handleCommand() {
  if (!serialBT->available()) {
    return;
  }

  String receivedData = serialBT->readString();
  Serial.println("Bluetooth data received: " + receivedData);
  serialBT->println("Data received: " + receivedData);

  vector<string> parts = this->splitString(&receivedData);

  if (parts.empty()) {
    Serial.println("Not processing bluetooth command: zero parts");
    return;
}

  if (parts.size() < 1 && parts.size() > 3) {
    Serial.println("Not processing bluetooth command: less than 1 or more than 3 parts");
    return;
  }

  string commandType = trim(parts.at(0));

  MemoryValue* memoryData = nullptr;

  if (commandType == "SET" || commandType == "GET") {
    string property = trim(parts.at(1));
    
    auto it = settingsMemory.find(property);

    if (it == settingsMemory.end()) {
      Serial.println("Invalid property");
      serialBT->println("Invalid property");
      return;
    }

    memoryData = it->second;
  }

  if (commandType == "SET") {
    handleSetCommand(memoryData, stoi(trim(parts.at(2))));
  } else if (commandType == "GET") {
    handleGetCommand(memoryData);
  } else if (commandType == "GET_LOGS") {
    handleGetLogsCommand();
  } else if (commandType == "GET_TEMPERATURE") {
    handleGetTemperatureCommand();
  } else if (commandType == "SET_APP_MODE_AUTO") {
    handleSetAppModeAutoCommand();
  } else if (commandType == "SET_APP_MODE_MANUAL") {
    handleSetAppModeManualCommand();
  } else if (commandType == "CLEAR_WARNINGS") {
    handleClearWarningsCommand();
  } else {
    handleInvalidCommand();
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
    serialBT->println("Invalid command: parts less than 1 or more than 3");

    return vector<string>();
  }

  string commandType = trim(parts.at(0));

  auto it = find(commandTypes.begin(), commandTypes.end(), commandType);

  if (it == commandTypes.end()) {
    Serial.println("Invalid command: invalid command type");
    serialBT->println("Invalid command: invalid command type");

    return vector<string>();
  }

  if (
    (commandType == "GET_LOGS" || commandType == "GET_TEMPERATURE" || commandType == "SET_APP_MODE_AUTO" || commandType == "SET_APP_MODE_MANUAL")
    && parts.size() != 1
  ) {
    Serial.println("Invalid command: Command expects to be the only one part");
    serialBT->println("Invalid command: Command expects to be the only one part");

    return vector<string>();
  }

  if (commandType == "SET" && parts.size() != 3) {
    Serial.println("Invalid command: SET parts different from 3");
    serialBT->println("Invalid command: SET parts different from 3");

    return vector<string>();
  }

  if (commandType == "GET" && parts.size() != 2) {
    Serial.println("Invalid command: GET parts different from 2");
    serialBT->println("Invalid command: GET parts different from 2");

    return vector<string>();
  }

  return parts;
}

string BluetoothWrapper::trim(const string& str) {
  size_t first = str.find_first_not_of(" \t\n\r");
  size_t last = str.find_last_not_of(" \t\n\r");
  return (first == string::npos || last == string::npos) ? "" : str.substr(first, (last - first + 1));
}

void BluetoothWrapper::handleSetCommand(MemoryValue* memoryData, int value) {
  memoryData->setValue(value);

  Serial.println("Value saved:");
  Serial.println(value);
  serialBT->println("Value saved:");
  serialBT->println(value);
}

void BluetoothWrapper::handleGetCommand(MemoryValue* memoryData) {
  int memoryValue = memoryData->readValue();

  Serial.println("Value read:");
  Serial.println(memoryValue);
  serialBT->println("Value read:");
  serialBT->println(memoryValue);
}

void BluetoothWrapper::handleGetLogsCommand() {
  vector<Log> lastLogs = getLastLogs(10);

  for (const auto& log: lastLogs) {
      Serial.print("Temperature: ");
      Serial.println(log.temperature);
      Serial.print("WindowOpening: ");
      Serial.println(log.windowOpening);
      Serial.print("DeltaTemporaryWindowOpening: ");
      Serial.println(log.deltaTemporaryWindowOpening);
      Serial.println("----------------");

      serialBT->print("Temperature: ");
      serialBT->println(log.temperature);
      serialBT->print("WindowOpening: ");
      serialBT->println(log.windowOpening);
      serialBT->print("DeltaTemporaryWindowOpening: ");
      serialBT->println(log.deltaTemporaryWindowOpening);
      serialBT->println("----------------");

    }
}

void BluetoothWrapper::handleGetTemperatureCommand() {
  float temperature = bme->readTemperature();

  Serial.println(temperature);
  serialBT->println(temperature);
}

void BluetoothWrapper::handleSetAppModeAutoCommand() {
  AppMode = Auto;

  Serial.println("AppMode changed to Auto");
  serialBT->println("AppMode changed to Auto");
}

void BluetoothWrapper::handleSetAppModeManualCommand() {
  AppMode = Manual;

  Serial.println("AppMode changed to Manual");
  serialBT->println("AppMode changed to Manual");
}

void BluetoothWrapper::handleClearWarningsCommand() {
  backgroundApp->clearWarnings();

  Serial.println("Warnings cleared");
  serialBT->println("Warnings cleared");
}

void BluetoothWrapper::handleInvalidCommand() {
  Serial.println("Invalid command");
  serialBT->println("Invalid command");
}