#include <unordered_map>
#include <string>
#include <sstream>
#include <Arduino.h>
#include <bluetoothWrapper.h>
#include <secrets.h>
#include <memoryData.h>
#include <memoryValue.h>

using namespace std;

unordered_map<string, MemoryValue*> settingsMemory;

BluetoothWrapper::BluetoothWrapper(BluetoothSerial* serialBT): serialBT(serialBT) {}

void BluetoothWrapper::init() {
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
}

void BluetoothWrapper::handleCommand() {
  if (!serialBT->available()) {
    return;
  }

  String receivedData = serialBT->readString();
  Serial.println("Bluetooth data received: " + receivedData);
  serialBT->println("Data received: " + receivedData);

  vector<string> parts = this->splitString(&receivedData);

  if (parts.size() != 2 && parts.size() != 3) {
    Serial.println("Not processing bluetooth command as it is invalid");
    return;
  }

  string commandType = trim(parts.at(0));
  string property = trim(parts.at(1));
  
  auto it = settingsMemory.find(property);

  if (it == settingsMemory.end()) {
    Serial.println("Invalid property");
    serialBT->println("Invalid property");
    return;
  }

  MemoryValue* memoryData = it->second;

  if (commandType == "SET") {
    int value = stoi(trim(parts.at(2)));

    memoryData->setValue(value);

    Serial.println("Value saved:");
    Serial.println(value);
    serialBT->println("Value saved:");
    serialBT->println(value);
  } else if (commandType == "GET") {
    int memoryValue = memoryData->readValue();

    Serial.println("Value read:");
    Serial.println(memoryValue);
    serialBT->println("Value read:");
    serialBT->println(memoryValue);
  } else {
    Serial.println("Invalid command");
    serialBT->println("Invalid command");
  }
}

vector<string> BluetoothWrapper::splitString(const String* command) {
  vector<string> parts;
  stringstream ss(command->c_str());
  string token;

  while (getline(ss, token, ' ')) {
    parts.push_back(token);
  }

  if (parts.size() < 2 || parts.size() > 3) {
    Serial.println("Invalid command: parts less than 2 or more than 3");
    serialBT->println("Invalid command: parts less than 2 or more than 3");

    return vector<string>();
  }

  if (parts[0] != "SET" && parts[0] != "GET") {
    Serial.println("Invalid command: invalid command type");
    serialBT->println("Invalid command: invalid command type");

    return vector<string>();
  }

  if (parts[0] == "SET" && parts.size() != 3) {
    Serial.println("Invalid command: SET parts different from 3");
    serialBT->println("Invalid command: SET parts different from 3");

    return vector<string>();
  }

  if (parts[0] == "GET" && parts.size() != 2) {
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