#include <Arduino.h>
#include <buttonHandler.h>

ButtonHandler::ButtonHandler(byte buttonGpio) {
    this->lastButtonState = false;
    this->buttonGpio = buttonGpio;
    this->pressCallback = nullptr;
}

void ButtonHandler::initialize() {
    pinMode(buttonGpio, INPUT_PULLUP);
}

void ButtonHandler::attachButtonPressCallback(void (*callback)()) {
    this->pressCallback = callback;
}

void ButtonHandler::checkButtonPress() {
    bool newButtonState = digitalRead(buttonGpio);
    unsigned long currentTime = micros();

    if (lastButtonState != newButtonState && currentTime - lastCompareMicros >= 100 * 1000) {
        if (!newButtonState) {
            if (pressCallback != nullptr) {
                pressCallback();
            }
        }

        lastButtonState = newButtonState;
        lastCompareMicros = currentTime;
    }
}