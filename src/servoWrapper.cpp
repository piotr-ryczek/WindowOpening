#include <Arduino.h>
#include <stdexcept>
#include <ESP32Servo.h>
#include <servoWrapper.h>

const uint8_t moveSpeedDelay = 5; // miliseconds

ServoWrapper::ServoWrapper(byte servoGpio, Servo& servo, MemoryValue& minMemoryValue, MemoryValue& maxMemoryValue): servo(servo), minMemoryValue(minMemoryValue), maxMemoryValue(maxMemoryValue) {
    this->servoGpio = servoGpio;
    this->min = 0;
    this->max = 180;
};

void ServoWrapper::initialize(int timerNumber) {
    ESP32PWM::allocateTimer(timerNumber);
    servo.setPeriodHertz(50);
    servo.attach(servoGpio);

    if (minMemoryValue.isSet()) {
        uint8_t minValue = minMemoryValue.readValue();
        this->setMin(minValue);
    }

    if (maxMemoryValue.isSet()) {
        uint8_t maxValue = maxMemoryValue.readValue();
        this->setMax(maxValue);
    }

    uint8_t middlePosition = abs(this->max - this->min) / 2;
    this->moveTo(middlePosition);
}

void ServoWrapper::setMin(uint8_t newMin) {
    min = newMin;

    minMemoryValue.setValue(newMin);
}

void ServoWrapper::setMax(uint8_t newMax) {
    max = newMax;

    maxMemoryValue.setValue(newMax);
}

void ServoWrapper::write(uint8_t newPositionDegrees) {
    servo.write(newPositionDegrees);
}

/**
 * In range 0 - 100 (has to be calibrated first)
 */
void ServoWrapper::moveTo(uint8_t newPosition) {
    uint8_t degrees = translateFrom100ToDegrees(newPosition);

    write(degrees);
}

void ServoWrapper::moveSmoothly() {
    if (this->movingSmoothlyCurrentPosition == this->movingSmoothlyTarget) {
        this->isMovingSmoothly = false;
        return;
    }

    if (this->movingSmoothlyCurrentPosition > this->movingSmoothlyTarget) {
        this->movingSmoothlyCurrentPosition--;
    } else {
        this->movingSmoothlyCurrentPosition++;
    }

    this->moveTo(this->movingSmoothlyCurrentPosition);
}

void ServoWrapper::setMovingSmoothlyTarget(uint8_t newPosition) {
    uint8_t currentPositionInDegrees = servo.read();
    uint8_t currentPosition = translateFromDegreesTo100(currentPositionInDegrees);

    this->movingSmoothlyTarget = newPosition;
    this->movingSmoothlyCurrentPosition = currentPosition;

    if (this->movingSmoothlyTarget == this->movingSmoothlyCurrentPosition) {
        return;
    }

    this->isMovingSmoothly = true;
}

uint8_t ServoWrapper::translateFrom100ToDegrees(uint8_t position) {
    // Should not happen
    if (min == max) {
        return min;
    }

    if (max > min) {
        return min + ((max - min) * position / 100);
    } else {
        return min - ((min - max) * position / 100);
    }
}

uint8_t ServoWrapper::translateFromDegreesTo100(uint8_t position) {
    // Should not happen
    if (min == max) {
        return 100;
    }

    uint8_t distance;
    uint8_t positionDiff;

    if (max > min) {
        if (position < min) {
            position = min;
        }

        if (position > max) {
            position = max;
        }

        distance = max - min;
        positionDiff = position - min;
    } else {
        if (position > min) {
            position = min;
        }

        if (position < max) {
            position = max;
        }

        distance = min - max;
        positionDiff = min - position;
    }

    return positionDiff * 100 / distance;

}