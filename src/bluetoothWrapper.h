#ifndef BLUETOOTH_WRAPPER_H
#define BLUETOOTH_WRAPPER_H

#include <vector>
#include <BluetoothSerial.h>

using namespace std;

class BluetoothWrapper {
  private:
    BluetoothSerial* serialBT;
    vector<string> splitString(const String* command);
    string trim(const string& str);

  public:
    BluetoothWrapper(BluetoothSerial* serialBT);
    void init();
    void handleCommand();
};

#endif