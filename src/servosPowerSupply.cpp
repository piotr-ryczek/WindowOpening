#include "servosPowerSupply.h"

const int POWER_ON = LOW;
const int POWER_OFF = HIGH;

ServosPowerSupply::ServosPowerSupply(byte servosPowerSupplyGpio) {
    this->servosPowerSupplyGpio = servosPowerSupplyGpio;
    this->currentState = POWER_OFF;
    this->targetState = POWER_OFF;
    this->turningOffCommandTime = 0;
}

void ServosPowerSupply::initialize() {
    pinMode(servosPowerSupplyGpio, OUTPUT);
    digitalWrite(servosPowerSupplyGpio, this->currentState);
}

void ServosPowerSupply::turnOn() {
    if (this->currentState == POWER_OFF) {
        Serial.println("Turning ON servos power supply");
    }

    this->targetState = POWER_ON;
    this->currentState = POWER_ON;
    digitalWrite(servosPowerSupplyGpio, this->currentState);
}

void ServosPowerSupply::turnOff() {
    Serial.println("Turning OFF servos power supply");

    this->currentState = POWER_OFF;
    digitalWrite(servosPowerSupplyGpio, this->currentState);
}

void ServosPowerSupply::turnOffDelayed() {
    this->targetState = POWER_OFF;
    this->turningOffCommandTime = millis();
}

void ServosPowerSupply::checkTargetState() {
    if (this->currentState == this->targetState) {
        return;
    }

    if (this->targetState == POWER_OFF) {
        if (millis() - this->turningOffCommandTime > SERVO_POWER_SUPPLY_DELAY) {
            this->turnOff();
        }
    }
}