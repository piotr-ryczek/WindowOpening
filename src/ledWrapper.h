#ifndef LED_WRAPPER_H
#define LED_WRAPPER_H

#include <Arduino.h>

#define PWM_LED_FREQUENCY   12000
#define PWM_LED_RESOLUTION  8

class LedWrapper {
    private:
        byte redGpio;
        byte greenGpio;
        byte blueGpio;
        uint8_t redChannel;
        uint8_t greenChannel;
        uint8_t blueChannel;

    public:
        LedWrapper(uint8_t redChannel, byte redGpio, uint8_t greenChannel, byte greenGpio, uint8_t blueChannel, byte blueGpio);
        void initialize();

        void setColor(uint8_t red, uint8_t green, uint8_t blue);
        void setNoColor();
        void setColorWhite();
        void setColorRed();
        void setColorBlue();
        void setColorGreen();
        void setColorYellow();
        void setColorPurple();
        void setColorLightBlue();
};

#endif