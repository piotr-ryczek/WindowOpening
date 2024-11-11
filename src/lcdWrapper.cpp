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
  }

}

void LcdWrapper::backlight() {
  this->lcd->backlight();
}

void LcdWrapper::noBacklight() {
  this->lcd->noBacklight();
}

void LcdWrapper::init() {
  this->lcd->init();
}

void LcdWrapper::clear() {
  this->topRowText = "";
  this->bottomRowText = "";
  this->lcd->clear();
}

void LcdWrapper::print(String topRowText, String bottomRowText) {
  if (topRowText == this->topRowText && bottomRowText == this->bottomRowText) {
    return;
  }

  this->topRowText = topRowText;
  this->bottomRowText = bottomRowText;
  this->scrollPosition = 0;

  this->lcd->clear();
  this->lcd->setCursor(0, 0);
  this->lcd->print(this->topRowText);
  this->lcd->setCursor(0, 1);
  this->lcd->print(this->bottomRowText);
}

void LcdWrapper::print(String topRowText) {
  if (topRowText == this->topRowText && this->bottomRowText == "") {
    return;
  }

  this->topRowText = topRowText;
  this->bottomRowText = "";
  this->scrollPosition = 0;

  this->lcd->clear();
  this->lcd->setCursor(0, 0);
  this->lcd->print(this->topRowText);
}

