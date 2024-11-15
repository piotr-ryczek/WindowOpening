#include <EEPROM.h>
#include <memoryValue.h>

MemoryValue::MemoryValue(int setAddress, int valueAddress): setAddress(setAddress), valueAddress(valueAddress) {}
MemoryValue::MemoryValue(int setAddress, int valueAddress, int defaultValue): setAddress(setAddress), valueAddress(valueAddress) {
    if (!this->isSet()) {
        this->setValue(defaultValue);
    }
}

void MemoryValue::setValue(int newValue) {
    EEPROM.writeInt(valueAddress, newValue);

    if (!isSet()) {
        EEPROM.writeBool(setAddress, false); // Reversed logic
    }

    EEPROM.commit();
}

int MemoryValue::readValue() {
    return EEPROM.readInt(valueAddress);
}

bool MemoryValue::isSet() {
    return EEPROM.readBool(setAddress) == false; // Reversing, 1 = Uninitialized / 0 = Initialized
}

void MemoryValue::unset() {
    EEPROM.writeBool(setAddress, true); // Reversed logic
    EEPROM.commit();
}