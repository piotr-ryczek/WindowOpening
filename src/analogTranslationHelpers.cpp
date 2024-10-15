#include <Arduino.h>

uint8_t translateAnalogToServo(uint16_t value) {
    return value * 180 / 4095;
}

uint8_t translateAnalogTo100Range(uint16_t value) {
    return value * 100 / 4095;
}