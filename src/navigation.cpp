#include <Arduino.h>
#include <cmath>
#include <navigation.h>
#include <analogTranslationHelpers.h>

const uint16_t deadPotentiometerMargin = 100; // In range 0-4095

vector<Setting> settings {
    Setting{
        name: SettingOptimalTemperature,
        valueMin: 10,
        valueMax: 25,
        memoryValue: &optimalTemperatureMemory,
    },
    Setting{
        name: SettingChangeDiffThreshold,
        valueMin: 0,
        valueMax: 100,
        memoryValue: &changeDiffThresholdMemory,
    },
};

Navigation::Navigation(byte potentiometerGpio, ServoWrapper& servoPullOpen, ServoWrapper& servoPullClose, LedWrapper& led, AppModeEnum* appMode): servoPullOpen(servoPullOpen), servoPullClose(servoPullClose), led(led) {
    this->appMainState = Sleep;
    this->mainMenuState = MainMenuNone; // Chosen menu
    this->mainMenuTemporaryState = MainMenuNone; // Temporary position while selecting
    this->mainMenuTemporaryAnalogPosition = -1; // By default unset; 0-4095 (Analog value)
    this->isMenuSelectionActivated = false;
    this->potentiometerGpio = potentiometerGpio;
    this->calibrationStep = CalibrationStepMin;
    this->selectedServo = &this->servoPullOpen;
    this->selectedServoEnum = ServoPullOpen;
    this->temporarySelectedServoEnum = ServoPullOpen; // Temporary value while rotating potentiometer
    this->selectedSettingEnum = SettingOptimalTemperature;
    this->temporarySelectedSettingEnum = SettingOptimalTemperature;
    this->temporaryAppMode = Manual; // Temporary value while rotating potentiometer
    this->appMode = appMode;
    this->selectedSetting = nullptr;

    this->assignRangesForMainMenu(vector<MainMenuEnum> { MainMenuCalibration, MainMenuMove, MainMenuMoveBothServos, MainMenuMoveSmoothly, MainMenuMoveBothServosSmoothly, MainMenuServoSelection, MainMenuAppMode, MainMenuSettings });
    this->assignRangesForSettings(vector<SettingEnum> { SettingOptimalTemperature, SettingChangeDiffThreshold });

    this->servoSelectionPositions = {
        ServoSelectionPosition{
            name: ServoPullOpen,
            from: 0,
            to: 50
        },
        ServoSelectionPosition{
            name: ServoPullClose,
            from: 51,
            to: 100
        }
    };

    this->appModeSelectionPositions = {
        AppModeSelectionPosition{
            name: Manual,
            from: 0,
            to: 50
        },
        AppModeSelectionPosition{
            name: Auto,
            from: 51,
            to: 100
        }
    };
}

void Navigation::assignRangesForMainMenu(const vector<MainMenuEnum>& positions) {
    if (positions.empty()) {
        throw std::runtime_error("Position cannot be empty");
    }

    int step = 100 / positions.size(); 

    int index = 0;
    for (auto& position : positions) {
        uint16_t from = index == 0 ? 0 : ceil(index * step);
        uint16_t to = index == positions.size() - 1 ? 100 : ceil((index + 1) * step) - 1;

        this->mainMenuPositions.push_back(MainMenuPosition{
            name: position,
            from: from,
            to: to,
        });

        index++;
    }
}

// Duplication from MainMenu ranges
void Navigation::assignRangesForSettings(const vector<SettingEnum>& positions) {
    if (positions.empty()) {
        throw std::runtime_error("Position cannot be empty");
    }

    int step = 100 / positions.size(); 

    int index = 0;
    for (auto& position : positions) {
        uint16_t from = index == 0 ? 0 : ceil(index * step);
        uint16_t to = index == positions.size() - 1 ? 100 : ceil((index + 1) * step) - 1;

        this->settingSelectionPositions.push_back(SettingPosition{
            name: position,
            from: from,
            to: to,
        });

        index++;
    }
}

void Navigation::logAppState() {
    Serial.print("AppMode: ");
    Serial.println(translateAppModeEnumToString(*appMode));
    Serial.print("AppMainState: ");
    Serial.println(translateAppMainStateEnumIntoString(appMainState));
    Serial.print("mainMenuState: ");
    Serial.println(translateMainMenuStateEnumIntoString(mainMenuState));
    Serial.print("MenuSelectionActivated: ");
    Serial.println(isMenuSelectionActivated);
    Serial.print("SelectedServo: ");
    Serial.println(translateServoEnumToString(selectedServoEnum));
}

uint16_t Navigation::getPotentiometerValue() {
    return analogRead(this->potentiometerGpio);
}

void Navigation::handleForward() {
    switch(appMainState) {
        case Sleep: {
            appMainState = Awaken;
            led.setNoColor();

            this->activateMenuChoosing();

            break;
        }
            
        
        case Awaken: {
            switch (mainMenuState) {
                case MainMenuNone: {
                    this->confirmMenuSelection();
                    this->deactivateMenuChoosing();

                    break;
                }       

                case MainMenuCalibration: {
                    switch (calibrationStep) {
                        case CalibrationStepMin: {
                            this->calibrationStep = CalibrationStepMax;
                            this->setServoCalibrationMin();
                            break;
                        }

                        case CalibrationStepMax: {
                            this->calibrationStep = CalibrationStepMin;
                            this->setServoCalibrationMax();

                            // Back to Main Menu
                            mainMenuState = MainMenuNone;
                            this->activateMenuChoosing();

                            break;
                        }
                    }

                    break;
                }

                case MainMenuServoSelection: {
                    this->confirmServoSelection();
                }

                case MainMenuAppMode: {
                    this->confirmAppModeSelection();
                    break;
                }

                case MainMenuMove:
                case MainMenuMoveBothServos: {
                    // Nothing
                    break;
                }

                case MainMenuMoveSmoothly: {
                    this->moveServoSmoothlyTo(); // Set new target
                    break;
                }

                case MainMenuMoveBothServosSmoothly: {
                    this->moveBothServoSmoothlyTo(); // Set new target
                    break;
                }

                case MainMenuSettings: {
                    if (this->selectedSetting == nullptr) {
                        this->confirmSettingSelection();
                    } else {
                        this->setSettingValue();
                    }
                    
                    break;
                }
            }   
        }
    }

    this->logAppState();
}

void Navigation::handleBackward() {
    switch(appMainState) {
        case Awaken: {
            switch (mainMenuState) {
                case MainMenuNone: {
                    appMainState = Sleep;
                    this->deactivateMenuChoosing();

                    break;
                }

                case MainMenuCalibration:
                case MainMenuMove:
                case MainMenuMoveSmoothly:
                case MainMenuAppMode:
                case MainMenuServoSelection: {
                    mainMenuState = MainMenuNone;
                    this->activateMenuChoosing();

                    break;
                }

                case MainMenuSettings: {
                    if (this->selectedSetting == nullptr) {
                        mainMenuState = MainMenuNone;
                        this->activateMenuChoosing();
                    } else {
                        this->selectedSetting = nullptr;
                    }

                    break;
                }
            }
        }
    }

    this->logAppState();
}

void Navigation::activateMenuChoosing() {
    this->isMenuSelectionActivated = true;
}

void Navigation::deactivateMenuChoosing() {
    this->isMenuSelectionActivated = false;
}

MainMenuEnum Navigation::findMenuSelection(uint8_t position) {
    for (const auto& menuPosition : mainMenuPositions) {
        if (position >= menuPosition.from && position <= menuPosition.to) {
            return menuPosition.name;
        }
    }

    return MainMenuNone;
}

ServoEnum Navigation::findServoSelection(uint8_t position) {
    for (const auto& servoSelectionPosition : servoSelectionPositions) {
        if (position >= servoSelectionPosition.from && position <= servoSelectionPosition.to) {
            return servoSelectionPosition.name;
        }
    }

    return ServoPullOpen;
}

AppModeEnum Navigation::findAppModeSelection(uint8_t position) {
    for (const auto& appModeSelectionPosition : appModeSelectionPositions) {
        if (position >= appModeSelectionPosition.from && position <= appModeSelectionPosition.to) {
            return appModeSelectionPosition.name;
        }
    }

    return Manual;
}

SettingEnum Navigation::findSettingSelection(uint8_t position) {
    for (const auto& settingSelectionPosition : settingSelectionPositions) {
        if (position >= settingSelectionPosition.from && position <= settingSelectionPosition.to) {
            return settingSelectionPosition.name;
        }
    }

    return SettingOptimalTemperature;
}


void Navigation::handleMenuSelection() {
    uint16_t analogValue = analogRead(this->potentiometerGpio);
    uint8_t menuPositionValue = translateAnalogTo100Range(analogValue);

    if (this->mainMenuTemporaryAnalogPosition == -1 || abs(analogValue - this->mainMenuTemporaryAnalogPosition) > deadPotentiometerMargin) {
        MainMenuEnum selectedMenuPosition = findMenuSelection(menuPositionValue);

        this->mainMenuTemporaryState = selectedMenuPosition;
        this->mainMenuTemporaryAnalogPosition = analogValue;
        displayMainMenuLed(this->mainMenuTemporaryState);
    }

    Serial.println(translateMainMenuStateEnumIntoString(mainMenuTemporaryState));
}

void Navigation::confirmMenuSelection() {
    mainMenuState = mainMenuTemporaryState;

    displayMainMenuLed(mainMenuState);
}

void Navigation::handleCalibrate() {
    uint16_t value = getPotentiometerValue();
    uint8_t servoValue = translateAnalogToServo(value);

    calibrationTemporaryValue = servoValue;

    Serial.println(servoValue);

    selectedServo->write(calibrationTemporaryValue);
}

void Navigation::handleMove() {
    uint16_t value = getPotentiometerValue();
    uint8_t servoValue = translateAnalogTo100Range(value);

    Serial.println(servoValue);

    selectedServo->moveTo(servoValue);
}

void Navigation::handleMoveBothServos() {
    uint16_t value = getPotentiometerValue();
    uint8_t servoValue = translateAnalogTo100Range(value);

    Serial.println(servoValue);

    servoPullOpen.moveTo(servoValue);
    servoPullClose.moveTo(servoValue);
}

void Navigation::handleServoSelection() {
    uint16_t value = getPotentiometerValue();
    uint8_t servoSelectionValue = translateAnalogTo100Range(value);

    temporarySelectedServoEnum = findServoSelection(servoSelectionValue);

    Serial.println(translateServoEnumToString(temporarySelectedServoEnum));
}

void Navigation::handleAppModeSelection() {
    uint16_t value = getPotentiometerValue();
    uint8_t appModeSelectionValue = translateAnalogTo100Range(value);

    temporaryAppMode = findAppModeSelection(appModeSelectionValue);

    Serial.println(translateAppModeEnumToString(temporaryAppMode));
}

void Navigation::handleSettingSelection() {
    uint16_t value = getPotentiometerValue();
    uint8_t settingSelectionValue = translateAnalogTo100Range(value);

    temporarySelectedSettingEnum = findSettingSelection(settingSelectionValue);

    Serial.println(translateSettingEnumToString(temporarySelectedSettingEnum));
}

void Navigation::handleSetSettingValue() {
    if (this->selectedSetting == nullptr) return;

    uint16_t value = getPotentiometerValue();
    uint32_t settingSelectionValue = translateAnalogToGivenRange(value, this->selectedSetting->valueMin, this->selectedSetting->valueMax);

    this->temporarySettingValue = settingSelectionValue;
}

void Navigation::confirmServoSelection() {
    selectedServoEnum = temporarySelectedServoEnum;

    switch (temporarySelectedServoEnum) {
        case ServoPullOpen:
            selectedServo = &servoPullOpen;
            break;
        case ServoPullClose:
            selectedServo = &servoPullClose;
            break;
    }
}

void Navigation::confirmAppModeSelection() {
    this->appMode = &temporaryAppMode;
}

void Navigation::confirmSettingSelection() {
    this->selectedSettingEnum = this->temporarySelectedSettingEnum;

    auto it = find_if(settings.begin(), settings.end(), [this](Setting setting) {
        return setting.name == this->selectedSettingEnum;
    });

    if (it != settings.end()) {
        this->selectedSetting = &(*it);

        Serial.println("Chosen setting:");
        Serial.println(translateSettingEnumToString(this->selectedSetting->name));
        Serial.println(this->selectedSetting->memoryValue->readValue());
    } else {
        Serial.println("Invalid setting selection");
    }
}

void Navigation::setSettingValue() {
    if (this->selectedSetting == nullptr) {
        throw std::runtime_error("Selected Setting is empty");
    }

    this->selectedSetting->memoryValue->setValue(this->temporarySettingValue);
}

void Navigation::setServoCalibrationMin() {
    uint16_t value = getPotentiometerValue();
    uint8_t servoValue = translateAnalogToServo(value); // 0 - 180

    selectedServo->setMin(servoValue);
}

void Navigation::setServoCalibrationMax() {
    uint16_t value = getPotentiometerValue();
    uint8_t servoValue = translateAnalogToServo(value); // 0 - 180

    selectedServo->setMax(servoValue);
}

void Navigation::moveServoSmoothlyTo() {
    uint16_t value = getPotentiometerValue();
    uint8_t servoPosition = translateAnalogTo100Range(value); // 0 - 100

    Serial.println(servoPosition);

    selectedServo->setMovingSmoothlyTarget(servoPosition);
}

void Navigation::moveBothServoSmoothlyTo() {
    uint16_t value = getPotentiometerValue();
    uint8_t servoPosition = translateAnalogTo100Range(value); // 0 - 100

    Serial.println(servoPosition);

    // No delay between movement beginning
    servoPullOpen.setMovingSmoothlyTarget(servoPosition);
    servoPullClose.setMovingSmoothlyTarget(servoPosition);
}


String Navigation::translateAppModeEnumToString(AppModeEnum appMode) {
    switch (appMode) {
        case Manual:
            return "Manual";
        case Auto:
            return "Auto";
    }

    return "Unknown";
}

String Navigation::translateAppMainStateEnumIntoString(AppMainStateEnum appMainState) {
    switch (appMainState) {
        case Sleep:
            return "Sleep";
        case Awaken:
            return "Awaken";
    }

    return "Unknown";
}

String Navigation::translateMainMenuStateEnumIntoString(MainMenuEnum mainMenuState) {
    switch (mainMenuState) {
        case MainMenuNone:
            return "None";
        case MainMenuCalibration:
            return "Calibration";
        case MainMenuMove:
            return "Move";
        case MainMenuMoveBothServos:
            return "MoveBothServos";
        case MainMenuMoveSmoothly:
            return "MoveSmoothly";
        case MainMenuMoveBothServosSmoothly:
            return "MoveBothServosSmoothly";
        case MainMenuAppMode:
            return "AppMode";
        case MainMenuSettings:
            return "Settings";
        case MainMenuServoSelection:
            return "Servo Selection";
    }

    return "Unknown";
}

String Navigation::translateServoEnumToString(ServoEnum servoEnum) {
    switch (servoEnum) {
        case ServoPullOpen:
            return "ServoPullOpen";
        case ServoPullClose:
            return "ServoPullClose";
    }

    return "Unknown";
}

String Navigation::translateSettingEnumToString(SettingEnum settingEnum) {
    switch (settingEnum) {
        case SettingOptimalTemperature:
            return "OptimalTemperature";
        case SettingChangeDiffThreshold:
            return "ChangeDiffThreshold";
    }

    return "Unknown";
}

void Navigation::displayMainMenuLed(MainMenuEnum mainMenuState) {
    switch (mainMenuState) {
        case MainMenuCalibration:
            led.setColorGreen();
            break;
        case MainMenuMove:
            led.setColorRed();
            break;
        case MainMenuMoveBothServos:
            led.setColorLightRed();
            break;
        case MainMenuMoveSmoothly:
            led.setColorPurple();
            break;
        case MainMenuMoveBothServosSmoothly:
            led.setColorLightPurple();
            break;
        case MainMenuAppMode:
            led.setColorBlue();
            break;
        case MainMenuServoSelection:
            led.setColorYellow();
            break;
        case MainMenuSettings:
            led.setColorOrange();
            break;
        case MainMenuNone:
        default:
            led.setColorWhite();
            break;
    }
}



