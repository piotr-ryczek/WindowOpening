#include "servosPowerSupply.h"

ServosPowerSupply::ServosPowerSupply(byte servosPowerSupplyGpio) {
    this->servosPowerSupplyGpio = servosPowerSupplyGpio;
    this->currentState = LOW;
    this->targetState = LOW;
    this->turningOffCommandTime = 0;
}

void ServosPowerSupply::initialize() {
    pinMode(servosPowerSupplyGpio, OUTPUT);
    digitalWrite(servosPowerSupplyGpio, this->currentState);
}

void ServosPowerSupply::turnOn() {
    if (this->currentState == LOW) {
        Serial.println("Turning ON servos power supply");
    }

    this->targetState = HIGH;
    this->currentState = HIGH;
    digitalWrite(servosPowerSupplyGpio, this->currentState);
}

void ServosPowerSupply::turnOff() {
    Serial.println("Turning OFF servos power supply");

    this->currentState = LOW;
    digitalWrite(servosPowerSupplyGpio, this->currentState);
}

void ServosPowerSupply::turnOffDelayed() {
    this->targetState = LOW;
    this->turningOffCommandTime = millis();
}

void ServosPowerSupply::checkTargetState() {
    if (this->currentState == this->targetState) {
        return;
    }

    if (this->targetState == LOW) {
        if (millis() - this->turningOffCommandTime > SERVO_POWER_SUPPLY_DELAY) {
            this->turnOff();
        }
    }
}