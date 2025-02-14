#include "batteryVoltageMeter.h"

BatteryVoltageMeter::BatteryVoltageMeter(byte batteryVoltageMeterPin, float batteryVoltage0Reference, float batteryVoltage100Reference) {
    this->batteryVoltageMeterPin = batteryVoltageMeterPin;
    this->batteryVoltage0Reference = batteryVoltage0Reference;
    this->batteryVoltage100Reference = batteryVoltage100Reference;
}

void BatteryVoltageMeter::init() {
    adc_chars = (esp_adc_cal_characteristics_t *)calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, VOLTAGE_REFERENCE, adc_chars);
}

float BatteryVoltageMeter::getVoltage() {
    int rawVoltageValue = analogRead(this->batteryVoltageMeterPin);
    uint32_t voltage = esp_adc_cal_raw_to_voltage(rawVoltageValue, adc_chars);
    float batteryVoltage = voltage * (RESISTOR_FIRST_VALUE + RESISTOR_SECOND_VALUE) / RESISTOR_SECOND_VALUE;

    return batteryVoltage;
}

float BatteryVoltageMeter::calculatePercentage(float batteryVoltage) {
    float batteryPercentage = (batteryVoltage - batteryVoltage0Reference) / (batteryVoltage100Reference - batteryVoltage0Reference) * 100;

    if (batteryPercentage < 0) {
        batteryPercentage = 0;
    } else if (batteryPercentage > 100) {
        batteryPercentage = 100;
    }

    return batteryPercentage;
}