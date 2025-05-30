#ifndef SERVO_WRAPPER_H
#define SERVO_WRAPPER_H

#include <Arduino.h>
#include <ESP32Servo.h>
#include <memoryValue.h>
#include <servosPowerSupply.h>
class ServoWrapper {
    private:
        byte servoGpio;
        Servo& servo;
        MemoryValue& minMemoryValue;
        MemoryValue& maxMemoryValue;
        ServosPowerSupply& servosPowerSupply;
        uint8_t readFromServo();
        uint8_t translateFrom100ToDegrees(uint8_t position);
        uint8_t translateFromDegreesTo100(uint8_t position);
        uint8_t movingSmoothlyTarget;
        uint8_t movingSmoothlyCurrentPosition;

    public:
        bool isMovingSmoothly = false;
        uint8_t min;
        uint8_t max;

        ServoWrapper(byte servoGpio, Servo& servo, MemoryValue& minMemoryValue, MemoryValue& maxMemoryValue, ServosPowerSupply& servosPowerSupply);
        void initialize(int timerNumber);
        void setMin(uint8_t newMin);
        void setMax(uint8_t newMax);
        void write(uint8_t newPositionDegrees); // Degrees 0 - 180
        void moveTo(uint8_t newPosition); // 0 - 100
        void moveSmoothly();
        void setMovingSmoothlyTarget(uint8_t newPosition); // 0 - 100
        uint8_t getCurrentPosition(); // 0-100

};

#endif