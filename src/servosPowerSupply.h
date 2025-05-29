#ifndef SERVO_POWER_SUPPLY_H
#define SERVO_POWER_SUPPLY_H

#include <Arduino.h>

const uint16_t SERVO_POWER_SUPPLY_DELAY = 2000;

class ServosPowerSupply {
    private:
        byte servosPowerSupplyGpio;
        byte targetState;
        void turnOff();
        void setState(byte state);
        unsigned long turningOffCommandTime;

    public:
        byte currentState;
        ServosPowerSupply(byte servoGpio);
        void initialize();
        void turnOn();
        void turnOffDelayed();
        void checkTargetState();
};

#endif