#include <Arduino.h>

// Translate the potentiometer value to a 0-180 range
uint8_t translateAnalogToServo(uint16_t value) {
    return value * 180 / 4095;
}

// Translate the potentiometer value to a 0-100 range
uint8_t translateAnalogTo100Range(uint16_t value) {
    return value * 100 / 4095;
}

uint32_t translateAnalogToGivenRange(uint16_t value, uint32_t rangeMin, uint32_t rangeMax) {
    uint32_t rangeDiff = rangeMax - rangeMin;

    float proportion = static_cast<float>(value) / 4095.0f;

    return rangeMin + static_cast<uint32_t>(proportion * rangeDiff);
}