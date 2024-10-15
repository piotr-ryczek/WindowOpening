#ifndef ANALOG_TRANSLATION_HELPERS_H
#define ANALOG_TRANSLATION_HELPERS_H

#include <Arduino.h>

uint8_t translateAnalogToServo(uint16_t value);
uint8_t translateAnalogTo100Range(uint16_t value);

#endif