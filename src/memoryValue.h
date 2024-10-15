#ifndef MEMORY_VALUE_H
#define MEMORY_VALUE_H

#include <Arduino.h>

class MemoryValue {
    private:
        int setAddress;
        int valueAddress;

    public:
        MemoryValue(int setAddress, int valueAddress);
        void setValue(uint8_t value);
        uint8_t readValue();

        bool isSet();
        void unset();
};

#endif