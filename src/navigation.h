#ifndef NAVIGATION_H
#define NAVIGATION_H

#include <Arduino.h>
#include <vector>
#include <servoWrapper.h>
#include <config.h>
#include <ledWrapper.h>
#include <memoryValue.h>
#include <memoryData.h>
#include <lcdWrapper.h>

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
    MainMenuServoSelection,
    MainMenuSettings,
};
enum CalibrationStepEnum { CalibrationStepMin, CalibrationStepMax };
enum ServoEnum { ServoPullOpen, ServoPullClose};
enum SettingEnum { 
    SettingOptimalTemperature, 
    SettingChangeDiffThreshold, 
    WindowOpeningCalculationInterval 
};

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

struct Setting {
    SettingEnum name;
    uint16_t valueMin;
    uint16_t valueMax;
    MemoryValue* memoryValue;
};

struct SettingPosition {
    SettingEnum name;
    uint16_t from;
    uint16_t to;
};

extern vector<Setting> settings;

class Navigation {
    private:
        byte potentiometerGpio;
        vector<MainMenuPosition> mainMenuPositions;
        vector<SettingPosition> settingSelectionPositions;
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
        SettingEnum selectedSettingEnum;
        SettingEnum temporarySelectedSettingEnum;
        AppModeEnum* appMode;
        AppModeEnum temporaryAppMode;
        uint32_t temporarySettingValue;
        LcdWrapper* lcd;

        void setServoCalibrationMin();
        void setServoCalibrationMax();

        void activateMenuChoosing();
        void deactivateMenuChoosing();

        void assignRangesForMainMenu(const vector<MainMenuEnum>&);
        void assignRangesForSettings(const vector<SettingEnum>&);

        // Find
        MainMenuEnum findMenuSelection(uint8_t position);
        ServoEnum findServoSelection(uint8_t position);
        AppModeEnum findAppModeSelection(uint8_t position);
        SettingEnum findSettingSelection(uint8_t position);

        // Translation
        String translateAppMainStateEnumIntoString(AppMainStateEnum appMainState);
        String translateMainMenuStateEnumIntoString(MainMenuEnum mainMenuState);
        String translateServoEnumToString(ServoEnum servoEnum);
        String translateServoEnumToStringShort(ServoEnum servoEnum);
        String translateAppModeEnumToString(AppModeEnum appModeEnum);
        String translateSettingEnumToString(SettingEnum settingEnum);

        // Confirmations
        void confirmServoSelection();
        void confirmAppModeSelection();
        void confirmSettingSelection();
        void setSettingValue();

        // Servo
        void moveServoSmoothlyTo();
        void moveBothServosSmoothlyTo(); // There is no delay between move beginning (in opposite to AutoMode real windowOpening adaptation)

        // Other
        uint16_t getPotentiometerValue();
        void displayMainMenuLed(MainMenuEnum mainMenuState);
        void logAppState();
        Setting* getSettingByEnum(SettingEnum settingName);

    public:
        Navigation(byte potentiometerGpio, ServoWrapper& servoPullOpen, ServoWrapper& servoPullClose, LedWrapper& ledWrapper, AppModeEnum* appMode, LcdWrapper* lcd);

        AppMainStateEnum appMainState;
        MainMenuEnum mainMenuState;
        Setting* selectedSetting;

        bool isMenuSelectionActivated;

        void handleForward();
        void handleBackward();
        void handleMenuSelection();
        void confirmMenuSelection();
        void handleCalibrate();
        void handleMove();
        void handleMoveBothServos();
        void handleServoSelection();
        void handleAppModeSelection();
        void handleSettingSelection();
        void handleSetSettingValue();
        void handleMoveSmoothlySelection();
        void handleMoveBothServosSmoothlySelection();
};

#endif