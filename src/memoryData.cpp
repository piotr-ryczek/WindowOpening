#include <Arduino.h>
#include <memoryValue.h>
#include <pidController.h>
#include <memoryData.h>

// MemoryValues

// Pull Open Calibration Min
MemoryValue servoPullOpenCalibrationMinMemory(SERVO_PULL_OPEN_CALIBRATION_MIN_SET_ADDRESS, SERVO_PULL_OPEN_CALIBRATION_MIN_VALUE_ADDRESS);

// Pull Open Calibration  Max
MemoryValue servoPullOpenCalibrationMaxMemory(SERVO_PULL_OPEN_CALIBRATION_MAX_SET_ADDRESS, SERVO_PULL_OPEN_CALIBRATION_MAX_VALUE_ADDRESS);

// Pull Close Calibration Min
MemoryValue servoPullCloseCalibrationMinMemory(SERVO_PULL_CLOSE_CALIBRATION_MIN_SET_ADDRESS, SERVO_PULL_CLOSE_CALIBRATION_MIN_VALUE_ADDRESS);

// Pull Close Calibration Max
MemoryValue servoPullCloseCalibrationMaxMemory(SERVO_PULL_CLOSE_CALIBRATION_MAX_SET_ADDRESS, SERVO_PULL_CLOSE_CALIBRATION_MAX_VALUE_ADDRESS);

MemoryValue optimalTemperatureMemory(OPTIMAL_TEMPERATURE_SET_ADDRESS, OPTIMAL_TEMPERATURE_VALUE_ADDRESS, PIDController::DEFAULT_OPTIMAL_TEMPERATURE);