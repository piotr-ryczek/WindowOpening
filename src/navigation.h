#ifndef NAVIGATION_H
#define NAVIGATION_H

#include <Arduino.h>
#include <vector>
#include <servoWrapper.h>
#include <config.h>
#include <ledWrapper.h>

using namespace std;

enum AppMainStateEnum { Sleep, Awaken };
enum MainMenuEnum {
    MainMenuNone,
    MainMenuCalibration,
    MainMenuMove,
    MainMenuMoveSmoothly,
    MainMenuMoveBothServos,
    MainMenuMoveBothServosSmoothly,
    MainMenuAppMode,
    MainMenuServoSelection
};
enum CalibrationStepEnum { CalibrationStepMin, CalibrationStepMax };
enum ServoEnum { ServoPullOpen, ServoPullClose};

struct MainMenuPosition {
    MainMenuEnum name;
    uint16_t from;
    uint16_t to;
};

struct ServoSelectionPosition {
    ServoEnum name;
    uint16_t from;
    uint16_t to;
};

struct AppModeSelectionPosition {
    AppModeEnum name;
    uint16_t from;
    uint16_t to;
};

class Navigation {
    private:
        byte potentiometerGpio;
        vector<MainMenuPosition> mainMenuPositions;
        vector<ServoSelectionPosition> servoSelectionPositions;
        vector<AppModeSelectionPosition> appModeSelectionPositions;
        uint16_t mainMenuTemporaryAnalogPosition; // 0 - 4095
        MainMenuEnum mainMenuTemporaryState;
        CalibrationStepEnum calibrationStep;
        uint8_t calibrationTemporaryValue;
        ServoWrapper& servoPullOpen;
        ServoWrapper& servoPullClose;
        LedWrapper& led;
        ServoWrapper* selectedServo;
        ServoEnum selectedServoEnum;
        ServoEnum temporarySelectedServoEnum;
        AppModeEnum* appMode;
        AppModeEnum temporaryAppMode;
        void setServoCalibrationMin();
        void setServoCalibrationMax();

        void activateMenuChoosing();
        void deactivateMenuChoosing();

        void assignRangesForMainMenu(const vector<MainMenuEnum>&);

        // Find
        MainMenuEnum findMenuSelection(uint8_t position);
        ServoEnum findServoSelection(uint8_t position);
        AppModeEnum findAppModeSelection(uint8_t position);

        // Translation
        String translateAppMainStateEnumIntoString(AppMainStateEnum appMainState);
        String translateMainMenuStateEnumIntoString(MainMenuEnum mainMenuState);
        String translateServoEnumToString(ServoEnum servoEnum);
        String translateAppModeEnumToString(AppModeEnum appModeEnum);

        // Confirmations
        void confirmServoSelection();
        void confirmAppModeSelection();

        // Servo
        void moveServoSmoothlyTo();
        void moveBothServosSmoothlyTo();

        // Other
        uint16_t getPotentiometerValue();
        void displayMainMenuLed(MainMenuEnum mainMenuState);
        void logAppState();

    public:
        Navigation(byte potentiometerGpio, ServoWrapper& servoPullOpen, ServoWrapper& servoPullClose, LedWrapper& ledWrapper, AppModeEnum* appMode);

        AppMainStateEnum appMainState;
        MainMenuEnum mainMenuState;

        bool isMenuSelectionActivated;

        void handleForward();
        void handleBackward();
        void handleMenuSelection();
        void confirmMenuSelection();
        void handleCalibrate();
        void handleMove();
        void handleServoSelection();
        void handleAppModeSelection();
        void handleMoveSmoothlySelection();
};

#endif