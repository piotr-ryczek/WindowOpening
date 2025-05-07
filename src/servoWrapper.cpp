#include <Arduino.h>
#include <stdexcept>
#include <ESP32Servo.h>
#include <servoWrapper.h>

const uint8_t moveSpeedDelay = 5; // miliseconds

ServoWrapper::ServoWrapper(byte servoGpio, Servo& servo, MemoryValue& minMemoryValue, MemoryValue& maxMemoryValue, ServosPowerSupply& servosPowerSupply): servo(servo), minMemoryValue(minMemoryValue), maxMemoryValue(maxMemoryValue), servosPowerSupply(servosPowerSupply) {
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
    this->servosPowerSupply.turnOn();

    servo.write(newPositionDegrees);
    
    this->servosPowerSupply.turnOffDelayed();
}

uint8_t ServoWrapper::readFromServo() {
    int value = servo.read();

    if (value < 0) {
        return 0;
    } else if (value > 180) {
        return 180;
    }

    return value;
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
    uint8_t currentPosition = getCurrentPosition();

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

uint8_t ServoWrapper::translateFromDegreesTo100(uint8_t positionInDegrees) {
    // Should not happen
    if (min == max) {
        return 100;
    }

    uint8_t distance;
    uint8_t positionDiff;

    if (max > min) {
        if (positionInDegrees < min) {
            positionInDegrees = min;
        }

        if (positionInDegrees > max) {
            positionInDegrees = max;
        }

        distance = max - min;
        positionDiff = positionInDegrees - min;
    } else {
        if (positionInDegrees > min) {
            positionInDegrees = min;
        }

        if (positionInDegrees < max) {
            positionInDegrees = max;
        }

        distance = min - max;
        positionDiff = min - positionInDegrees;
    }

    return positionDiff * 100 / distance;
}

uint8_t ServoWrapper::getCurrentPosition() {
    uint8_t currentPositionInDegrees = readFromServo();
    uint8_t currentPosition = translateFromDegreesTo100(currentPositionInDegrees);

    return currentPosition;
}