#include <Arduino.h>
#include <cmath>
#include <navigation.h>
#include <analogTranslationHelpers.h>

const uint16_t deadPotentiometerMargin = 100; // In range 0-4095

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
    this->temporaryAppMode = Manual; // Temporary value while rotating potentiometer
    this->appMode = appMode;

    this->assignRangesForMainMenu(vector<MainMenuEnum> { MainMenuCalibration, MainMenuMove, MainMenuMoveBothServos, MainMenuMoveSmoothly, MainMenuMoveBothServosSmoothly, MainMenuServoSelection, MainMenuAppMode });

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

// this->mainMenuPositions = {
//         MainMenuPosition{
//             name: MainMenuCalibration,
//             from: 0,
//             to: 20
//         },
//         MainMenuPosition{
//             name: MainMenuMove,
//             from: 20,
//             to: 40
//         },
//         MainMenuPosition{
//             name: MainMenuMoveBothServos,
//             from: 20,
//             to: 40
//         },
//         MainMenuPosition{
//             name: MainMenuMoveSmoothly,
//             from: 40,
//             to: 60
//         },
//         MainMenuPosition{
//             name: MainMenuMoveBothServosSmoothly,
//             from: 40,
//             to: 60
//         },
//         MainMenuPosition{
//             name: MainMenuServoSelection,
//             from: 60,
//             to: 80
//         },
//         MainMenuPosition{
//             name: MainMenuAppMode,
//             from: 80,
//             to: 100
//         }
//     };

void Navigation::assignRangesForMainMenu(vector<MainMenuEnum> positions) {
    if (positions.empty()) {
        throw std::runtime_error("Position cannot be empty");
    }

    int step = 100 / positions.size();

    int index = 0;
    for (auto& position : positions) {
        this->mainMenuPositions.push_back(MainMenuPosition{
            name: position,
            from: ceil(index * step),
            to: ceil((index + 1) * step),
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

                case MainMenuMove: {
                    // Nothing
                    break;
                }

                case MainMenuMoveSmoothly: {
                    this->moveServoSmoothlyTo(); // Set new target
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

void Navigation::handleMoveSmoothlySelection() {
    // uint16_t value = getPotentiometerValue();
    // uint8_t servoValue = translateAnalogTo100Range(value);

    // Serial.println(servoValue);
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
        case MainMenuMoveSmoothly:
            return "MoveSmoothly";
        case MainMenuAppMode:
            return "AppMode";
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

void Navigation::displayMainMenuLed(MainMenuEnum mainMenuState) {
    switch (mainMenuState) {
        case MainMenuCalibration:
            led.setColorGreen();
            break;
        case MainMenuMove:
            led.setColorRed();
            break;
        case MainMenuMoveSmoothly:
            led.setColorPurple();
            break;
        case MainMenuAppMode:
            led.setColorBlue();
            break;
        case MainMenuServoSelection:
            led.setColorYellow();
            break;
        case MainMenuNone:
        default:
            led.setColorWhite();
            break;
    }
}



