#ifndef MEMORY_VALUE_H
#define MEMORY_VALUE_H

#include <Arduino.h>

class MemoryValue {
    private:
        int setAddress;
        int valueAddress;

    public:
        MemoryValue(int setAddress, int valueAddress);
        MemoryValue(int setAddress, int valueAddress, int defaultValue);
        void setValue(int value);
        int readValue();

        bool isSet();
        void unset();
};

#endif