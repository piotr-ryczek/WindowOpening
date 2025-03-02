#include <lcdWrapper.h>

LcdWrapper::LcdWrapper(LiquidCrystal_I2C* lcd): lcd(lcd) {}

void LcdWrapper::checkScroll() {
  if (this->topRowText == "" || millis() - lastPrintMiliseconds < printDelay) {
    return;
  }

  int topRowTextLength = this->topRowText.length();

  if (topRowTextLength > 16) {
    if (this->scrollPosition + 16 == topRowTextLength) {
      this->scrollPosition = 0;

      String finalBottomRowText = this->bottomRowText;

      for (int i = 0; i < topRowTextLength - 16; i++) {
        this->lcd->scrollDisplayRight();
        finalBottomRowText += " ";
      }

      this->lcd->setCursor(0, 1);
      this->lcd->print(finalBottomRowText);
    } else {
      this->scrollPosition++;
      this->lcd->scrollDisplayLeft();

      this->lcd->setCursor(this->scrollPosition, 1);
      this->lcd->print(this->bottomRowText);
    }

    this->lastPrintMiliseconds = millis();
  }
}

void LcdWrapper::backlight() {
  this->lcd->backlight();
}

void LcdWrapper::noBacklight() {
  this->lcd->noBacklight();
}

void LcdWrapper::turnOn() {
  this->lcd->init();
  this->lcd->backlight();
}

void LcdWrapper::turnOff() {
  this->lcd->noBacklight();
}

void LcdWrapper::initialize() {
  this->lcd->init();
}

void LcdWrapper::clear() {
  this->topRowText = "";
  this->bottomRowText = "";
  this->lcd->clear();
}

void LcdWrapper::clearTopRow() {
  this->topRowText = "";
  this->lcd->setCursor(0, 0);
  this->lcd->print("                                ");
}

void LcdWrapper::clearBottomRow() {
  this->bottomRowText = "";
  this->lcd->setCursor(0, 1);
  this->lcd->print("                                ");
}

void LcdWrapper::print(String topRowText, String bottomRowText) {
  if (topRowText == this->topRowText && bottomRowText == this->bottomRowText) {
    return;
  }

  this->lcd->home();
  this->lastPrintMiliseconds = millis();
  this->scrollPosition = 0;

  if (topRowText != this->topRowText) {
    this->clearTopRow();
    this->topRowText = topRowText;

    this->lcd->setCursor(0, 0);
    this->lcd->print(this->topRowText);
  }

  if (bottomRowText != this->bottomRowText) {
    this->clearBottomRow();
    this->bottomRowText = bottomRowText;

    this->lcd->setCursor(0, 1);
    this->lcd->print(this->bottomRowText);
  }
}

void LcdWrapper::print(String topRowText) {
  if (topRowText == this->topRowText && this->bottomRowText == "") {
    return;
  }

  this->lcd->home();
  this->lastPrintMiliseconds = millis();

  this->topRowText = topRowText;
  this->bottomRowText = ""; 
  this->scrollPosition = 0;

  this->lcd->clear();
  this->lcd->setCursor(0, 0);
  this->lcd->print(this->topRowText);
}

