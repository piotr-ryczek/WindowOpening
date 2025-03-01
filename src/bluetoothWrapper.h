#ifndef BLUETOOTH_WRAPPER_H
#define BLUETOOTH_WRAPPER_H

#include <vector>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <BluetoothSerial.h>
#include <BLEServer.h>

#include <memoryValue.h>
#include <backgroundApp.h>
#include <weatherLogs.h>
#include <servoWrapper.h>
#include <batteryVoltageMeter.h>
using namespace std;

class BluetoothWrapper {
  private:
    BLECharacteristic *pCharacteristic;
    Adafruit_BME280* bme;
    BackgroundApp* backgroundApp;
    ServoWrapper* servoPullOpen;
    ServoWrapper* servoPullClose;
    BatteryVoltageMeter* batteryVoltageMeterBox;
    BatteryVoltageMeter* batteryVoltageMeterServos;

    vector<string> splitString(const String* command);
    string trim(const string& str);

    String handleSetCommand(MemoryValue* memoryData, int value);
    String handleGetCommand(MemoryValue* memoryData);
    vector<String> handleGetLogsCommand();
    String handleGetTemperatureCommand();
    String handleSetAppModeAutoCommand();
    String handleSetAppModeManualCommand();
    String handleClearWarningsCommand();
    String handleGetLastWeatherLogCommand();
    String handleForceOpeningWindowCalculationCommand();
    String handleMoveBothServosSmoothlyTo(uint8_t newPosition);
    String handleGetBatteryVoltageCommand(BatteryVoltageMeter* batteryVoltageMeter);

    String handleInvalidCommand();

  public:
    BluetoothWrapper(Adafruit_BME280* bme, BackgroundApp* backgroundApp, ServoWrapper* servoPullOpen, ServoWrapper* servoPullClose, BatteryVoltageMeter* batteryVoltageMeterBox, BatteryVoltageMeter* batteryVoltageMeterServos);
    void init();
    tuple<vector<String>, String> handleCommand(String* message);
    void checkQueue();
};

#endif