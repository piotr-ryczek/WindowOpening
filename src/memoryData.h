#ifndef MEMORY_DATA_H
#define MEMORY_DATA_H

#include <Arduino.h>
#include <memoryValue.h>
#include <pidController.h>

// Memory Addresses

const int SERVO_PULL_OPEN_CALIBRATION_MIN_SET_ADDRESS = 0;
const int SERVO_PULL_OPEN_CALIBRATION_MIN_VALUE_ADDRESS = 4;

const int SERVO_PULL_OPEN_CALIBRATION_MAX_SET_ADDRESS = 8;
const int SERVO_PULL_OPEN_CALIBRATION_MAX_VALUE_ADDRESS = 12;

const int SERVO_PULL_CLOSE_CALIBRATION_MIN_SET_ADDRESS = 16;
const int SERVO_PULL_CLOSE_CALIBRATION_MIN_VALUE_ADDRESS = 20;

const int SERVO_PULL_CLOSE_CALIBRATION_MAX_SET_ADDRESS = 24;
const int SERVO_PULL_CLOSE_CALIBRATION_MAX_VALUE_ADDRESS = 28;

const int OPTIMAL_TEMPERATURE_SET_ADDRESS = 32;
const int OPTIMAL_TEMPERATURE_VALUE_ADDRESS = 36;

const int P_TERM_POSITIVE_SET_ADDRESS = 40;
const int P_TERM_POSITIVE_VALUE_ADDRESS = 44;

const int P_TERM_NEGATIVE_SET_ADDRESS = 48;
const int P_TERM_NEGATIVE_VALUE_ADDRESS = 52;

const int D_TERM_POSITIVE_SET_ADDRESS = 56;
const int D_TERM_POSITIVE_VALUE_ADDRESS = 60;

const int D_TERM_NEGATIVE_SET_ADDRESS = 64;
const int D_TERM_NEGATIVE_VALUE_ADDRESS = 68;

const int O_TERM_POSITIVE_SET_ADDRESS = 72;
const int O_TERM_POSITIVE_VALUE_ADDRESS = 76;

const int O_TERM_NEGATIVE_SET_ADDRESS = 80;
const int O_TERM_NEGATIVE_VALUE_ADDRESS = 84;

const int I_TERM_SET_ADDRESS = 88;
const int I_TERM_VALUE_ADDRESS = 92;

const int CHANGE_DIFF_THRESHOLD_SET_ADDRESS = 96;
const int CHANGE_DIFF_THRESHOLD_VALUE_ADDRESS = 100;

const int WINDOW_OPENING_CALCULATION_INTERVAL_SET_ADDRESS = 104;
const int WINDOW_OPENING_CALCULATION_INTERVAL_VALUE_ADDRESS = 108;

const int OPENING_TERM_POSITIVE_TEMPERATURE_INCREASE_SET_ADDRESS = 112;
const int OPENING_TERM_POSITIVE_TEMPERATURE_INCREASE_VALUE_ADDRESS = 116;


// MemoryValues

// Pull Open Calibration Min
extern MemoryValue servoPullOpenCalibrationMinMemory;

// Pull Open Calibration  Max
extern MemoryValue servoPullOpenCalibrationMaxMemory;

// Pull Close Calibration Min
extern MemoryValue servoPullCloseCalibrationMinMemory;

// Pull Close Calibration Max
extern MemoryValue servoPullCloseCalibrationMaxMemory;

// Settings
extern MemoryValue optimalTemperatureMemory;

extern MemoryValue pTermPositive;

extern MemoryValue pTermNegative;

extern MemoryValue dTermPositive;

extern MemoryValue dTermNegative;

extern MemoryValue oTermPositive;

extern MemoryValue oTermNegative;

extern MemoryValue iTerm;

extern MemoryValue changeDiffThresholdMemory;

extern MemoryValue windowOpeningCalculationIntervalMemory;

extern MemoryValue openingTermPositiveTemperatureIncrease;

#endif