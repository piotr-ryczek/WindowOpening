#ifndef BATTERY_VOLTAGE_METER_H
#define BATTERY_VOLTAGE_METER_H

#include <Arduino.h>
#include <esp_adc_cal.h>

#include "config.h"

class BatteryVoltageMeter {
    public:
        BatteryVoltageMeter(byte batteryVoltageMeterPin, float batteryVoltage0Reference, float batteryVoltage100Reference);
        float getVoltage();
        float calculatePercentage(float batteryVoltage);
        void init();
    private:
        byte batteryVoltageMeterPin;
        float batteryVoltage0Reference;
        float batteryVoltage100Reference;
        esp_adc_cal_characteristics_t *adc_chars;
};

#endif