#ifndef BLUETOOTH_WRAPPER_H
#define BLUETOOTH_WRAPPER_H

#include <vector>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <BluetoothSerial.h>

#include <memoryValue.h>

using namespace std;

class BluetoothWrapper {
  private:
    BluetoothSerial* serialBT;
    Adafruit_BME280* bme;
    vector<string> splitString(const String* command);
    string trim(const string& str);

    void handleSetCommand(MemoryValue* memoryData, int value);
    void handleGetCommand(MemoryValue* memoryData);
    void handleGetLogsCommand();
    void handleGetTemperatureCommand();
    void handleSetAppModeAutoCommand();
    void handleSetAppModeManualCommand();
    void handleInvalidCommand();

  public:
    BluetoothWrapper(BluetoothSerial* serialBT, Adafruit_BME280* bme);
    void init();
    void handleCommand();
};

#endif