#ifndef BLUETOOTH_WRAPPER_H
#define BLUETOOTH_WRAPPER_H

#include <vector>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <BluetoothSerial.h>

#include <memoryValue.h>
#include <backgroundApp.h>
#include <weatherLogs.h>

using namespace std;

class BluetoothWrapper {
  private:
    Adafruit_BME280* bme;
    BackgroundApp* backgroundApp;
    vector<string> splitString(const String* command);
    string trim(const string& str);

    String handleSetCommand(MemoryValue* memoryData, int value);
    String handleGetCommand(MemoryValue* memoryData);
    String handleGetLogsCommand();
    String handleGetTemperatureCommand();
    String handleSetAppModeAutoCommand();
    String handleSetAppModeManualCommand();
    String handleClearWarningsCommand();
    String handleInvalidCommand();
    String handleGetLastWeatherLogCommand();
    String handleForceOpeningWindowCalculationCommand();

  public:
    BluetoothWrapper(Adafruit_BME280* bme, BackgroundApp* backgroundApp);
    void init();
    String handleCommand(String* message);
};

#endif