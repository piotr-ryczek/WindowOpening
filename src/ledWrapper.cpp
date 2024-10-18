#include <ledWrapper.h>

LedWrapper::LedWrapper(uint8_t redChannel, byte redGpio, uint8_t greenChannel, byte greenGpio, uint8_t blueChannel, byte blueGpio) {
    this->redGpio = redGpio;
    this->greenGpio = greenGpio;
    this->blueGpio = blueGpio;

    this->redChannel = redChannel;
    this->greenChannel = greenChannel;
    this->blueChannel = blueChannel;
}

void LedWrapper::initialize() {
    ledcSetup(redChannel, PWM_LED_FREQUENCY, PWM_LED_RESOLUTION);
    ledcSetup(greenChannel, PWM_LED_FREQUENCY, PWM_LED_RESOLUTION);
    ledcSetup(blueChannel, PWM_LED_FREQUENCY, PWM_LED_RESOLUTION);

    ledcAttachPin(redGpio, redChannel);
    ledcAttachPin(greenGpio, greenChannel);
    ledcAttachPin(blueGpio, blueChannel);
}

void LedWrapper::setColor(uint8_t red, uint8_t green, uint8_t blue) {
    ledcWrite(redChannel, 255 - red);
    ledcWrite(greenChannel, 255 - green);
    ledcWrite(blueChannel, 255 - blue);
}

void LedWrapper::setNoColor() {
    setColor(0, 0, 0);
}

void LedWrapper::setColorWhite() {
    setColor(255, 255, 255);
}

void LedWrapper::setColorRed() {
    setColor(255, 0, 0);
}

void LedWrapper::setColorLightRed() {
    setColor(255, 122, 122);
}

void LedWrapper::setColorBlue() {
    setColor(0, 0, 255);
}

void LedWrapper::setColorGreen() {
    setColor(0, 255, 0);
}

void LedWrapper::setColorYellow() {
    setColor(255, 255, 0);
}

void LedWrapper::setColorPurple() {
    setColor(255, 0, 255);
}

void LedWrapper::setColorLightPurple() {
    setColor(255, 122, 255);
}

void LedWrapper::setColorLightBlue() {
    setColor(0, 255, 255);
}