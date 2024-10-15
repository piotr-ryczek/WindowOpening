#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <Arduino.h>

class ButtonHandler {
    private:
        bool lastButtonState;
        unsigned long lastCompareMicros; // Microseconds
        byte buttonGpio;
        void (*pressCallback)();   

    public:
        ButtonHandler(byte buttonGpio);
        void initialize();

        void attachButtonPressCallback(void (*callback)());

        void checkButtonPress();
};

#endif