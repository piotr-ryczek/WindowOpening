#ifndef BLUETOOTH_WRAPPER_H
#define BLUETOOTH_WRAPPER_H

#include <vector>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <BluetoothSerial.h>

#include <memoryValue.h>
#include <backgroundApp.h>

using namespace std;

class BluetoothWrapper {
  private:
    BluetoothSerial* serialBT;
    Adafruit_BME280* bme;
    BackgroundApp* backgroundApp;
    vector<string> splitString(const String* command);
    string trim(const string& str);

    void handleSetCommand(MemoryValue* memoryData, int value);
    void handleGetCommand(MemoryValue* memoryData);
    void handleGetLogsCommand();
    void handleGetTemperatureCommand();
    void handleSetAppModeAutoCommand();
    void handleSetAppModeManualCommand();
    void handleClearWarningsCommand();
    void handleInvalidCommand();

  public:
    BluetoothWrapper(BluetoothSerial* serialBT, Adafruit_BME280* bme, BackgroundApp* backgroundApp);
    void init();
    void handleCommand();
};

#endif