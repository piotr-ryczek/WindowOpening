#include <Arduino.h>

uint8_t translateAnalogToServo(uint16_t value) {
    return value * 180 / 4095;
}

uint8_t translateAnalogTo100Range(uint16_t value) {
    return value * 100 / 4095;
}

uint32_t translateAnalogToGivenRange(uint16_t value, uint32_t rangeMin, uint32_t rangeMax) {
    uint32_t rangeDiff = rangeMax - rangeMin;

    double proportion = value / 4095;

    return rangeMin + (proportion * rangeDiff);
}