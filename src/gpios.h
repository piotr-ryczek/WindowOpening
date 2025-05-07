#ifndef GPIOS_H
#define GPIOS_H

#include <Arduino.h>

const int LED_RED_PWM_TIMER_INDEX = 0;
const int LED_GREEN_PWM_TIMER_INDEX = 1;
const int LED_BLUE_PWM_TIMER_INDEX = 2;

const int SERVO_PULL_OPEN_PWM_TIMER_INDEX = 3;
const int SERVO_PULL_CLOSE_PWM_TIMER_INDEX = 4;

const byte ENTER_BUTTON_GPIO = 14;
const byte EXIT_BUTTON_GPIO = 12;
const byte POTENTIOMETER_GPIO = 34;
const byte SERVO_PULL_OPEN_GPIO = 21;
const byte SERVO_PULL_CLOSE_GPIO = 22;

const byte LED_RED_GPIO = 25;
const byte LED_GREEN_GPIO = 26;
const byte LED_BLUE_GPIO = 27;

const byte BME_280_SDA_GPIO = 18;
const byte BME_280_SCL_GPIO = 19;

const byte LCD_SDA_GPIO = 17;
const byte LCD_SCL_GPIO = 16;

const byte BATTERY_VOLTAGE_BOX_METER_GPIO = 32;
const byte BATTERY_VOLTAGE_SERVOS_METER_GPIO = 33;

const byte SERVOS_POWER_SUPPLY_GPIO = 23;

#endif