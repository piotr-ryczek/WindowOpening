#ifndef LCD_WRAPPER_H
#define LCD_WRAPPER_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

const int printDelay = 2000;

class LcdWrapper {
  private:
    LiquidCrystal_I2C* lcd;
    String topRowText = "";
    String bottomRowText = "";
    uint16_t scrollPosition = 0;
    uint16_t lastPrintMiliseconds = 0;

  public:
    LcdWrapper(LiquidCrystal_I2C* lcd);
    void checkScroll();
    void backlight();
    void noBacklight();
    void turnOn();
    void turnOff();
    void print(String topRowText);
    void print(String topRowText, String bottomRowText);
    void initialize();
    void clear();
    void clearBottomRow();
    void clearTopRow();
};

#endif