#ifndef MEMORY_DATA_H
#define MEMORY_DATA_H

#include <Arduino.h>
#include <memoryValue.h>
#include <pidController.h>

// Memory Addresses

const int SERVO_PULL_OPEN_CALIBRATION_MIN_SET_ADDRESS = 0;
const int SERVO_PULL_OPEN_CALIBRATION_MIN_VALUE_ADDRESS = 1;

const int SERVO_PULL_OPEN_CALIBRATION_MAX_SET_ADDRESS = 2;
const int SERVO_PULL_OPEN_CALIBRATION_MAX_VALUE_ADDRESS = 3;

const int SERVO_PULL_CLOSE_CALIBRATION_MIN_SET_ADDRESS = 4;
const int SERVO_PULL_CLOSE_CALIBRATION_MIN_VALUE_ADDRESS = 5;

const int SERVO_PULL_CLOSE_CALIBRATION_MAX_SET_ADDRESS = 6;
const int SERVO_PULL_CLOSE_CALIBRATION_MAX_VALUE_ADDRESS = 7;

const int OPTIMAL_TEMPERATURE_SET_ADDRESS = 8;
const int OPTIMAL_TEMPERATURE_VALUE_ADDRESS = 9;

// Unused currently
const int P_TERM_POSITIVE_SET_ADDRESS = 10;
const int P_TERM_POSITIVE_VALUE_ADDRESS = 11;

const int P_TERM_NEGATIVE_SET_ADDRESS = 12;
const int P_TERM_NEGATIVE_VALUE_ADDRESS = 13;

const int D_TERM_POSITIVE_SET_ADDRESS = 14;
const int D_TERM_POSITIVE_VALUE_ADDRESS = 15;

const int D_TERM_NEGATIVE_SET_ADDRESS = 16;
const int D_TERM_NEGATIVE_VALUE_ADDRESS = 17;

const int O_TERM_POSITIVE_SET_ADDRESS = 18;
const int O_TERM_POSITIVE_VALUE_ADDRESS = 19;

const int O_TERM_NEGATIVE_SET_ADDRESS = 20;
const int O_TERM_NEGATIVE_VALUE_ADDRESS = 21;

const int I_TERM_SET_ADDRESS = 22;
const int I_TERM_VALUE_ADDRESS = 23;


// MemoryValues

// Pull Open Calibration Min
extern MemoryValue servoPullOpenCalibrationMinMemory;

// Pull Open Calibration  Max
extern MemoryValue servoPullOpenCalibrationMaxMemory;

// Pull Close Calibration Min
extern MemoryValue servoPullCloseCalibrationMinMemory;

// Pull Close Calibration Max
extern MemoryValue servoPullCloseCalibrationMaxMemory;

extern MemoryValue optimalTemperatureMemory;

#endif