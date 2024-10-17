#include <iostream>
#include <vector>

#include "pidController.h"
#include "logs.h"

using namespace std;
using namespace PIDController;

namespace PIDController {
    const double OPTIMAL_TEMPERATURE = 22;
    const double P_TERM_POSITIVE = 25;
    const double P_TERM_NEGATIVE = 15;
    const double D_TERM_POSITIVE = 50;
    const double D_TERM_NEGATIVE = 45;

    // Factor pushing to open when diff with Optimal Temperature is relatively small
    const double O_TERM_POSITIVE = 5; // Opening Term
    const double O_TERM_NEGATIVE = 0.2; // Opening Term

    const double I_TERM = 4;

    const double OPENING_TERM_POSITIVE_TEMPERATURE_INCREASE = 5;

    const double CHANGE_DIFF_THRESHOLD = 20; // Don't change opening below this number to avoid to often window manipulation

    // PID

    double calculateProportionalTermValue(double newTemperature) {
        double temperatureDiffFromOptimal = newTemperature - OPTIMAL_TEMPERATURE;

        bool ifTemperatureAboveOptimal = temperatureDiffFromOptimal >= 0;

        const double P_TERM = ifTemperatureAboveOptimal ? P_TERM_POSITIVE : P_TERM_NEGATIVE;

        return temperatureDiffFromOptimal * P_TERM;
    }

    double calculateIntegralTermValue(double newTemperature) {
        double accumulatedDiff = 0;
        for (Log log: logs) {
            accumulatedDiff += log.temperature - OPTIMAL_TEMPERATURE;
        }

        return accumulatedDiff * I_TERM;
    }

    double calculateDerivativeTermValue(double newTemperature) {
        Log lastLog = logs.back();

        double temperatureDiffFromLastLog = newTemperature - lastLog.temperature;
        bool ifTemperatureIncreasing = temperatureDiffFromLastLog >= 0;

        const double D_TERM = ifTemperatureIncreasing ? D_TERM_POSITIVE : D_TERM_NEGATIVE;

        return temperatureDiffFromLastLog * D_TERM;
    }

    // Increasing opening factor if close to OPTIMAL_TEMPERATURE or far above it
    double calculateOpeningTermValue(double newTemperature) {
        double temperatureDiffFromOptimal = newTemperature - OPTIMAL_TEMPERATURE;

        double openingTermValue;

        // If diff is below 0 - decrease factor and make positive value
        if (temperatureDiffFromOptimal < 0) {
            openingTermValue = (1 / temperatureDiffFromOptimal) * O_TERM_NEGATIVE * -1;
        } else {
            openingTermValue = (temperatureDiffFromOptimal + OPENING_TERM_POSITIVE_TEMPERATURE_INCREASE) * O_TERM_POSITIVE;
        }

        return openingTermValue;
    }

    int limitFromExtremes(int opening) {
        if (opening < 0) {
            return 0;
        }
        
        if (opening > 100) {
            return 100;
        }

        return opening;
    }

    int calculateWindowOpening(double newTemperature) {
        // Retrieve last log to compare
        Log lastLog = logs.back();

        // Calculate all terms values
        double proportionalTermValue = calculateProportionalTermValue(newTemperature); // Reacting to difference size
        double integralTermValue = calculateIntegralTermValue(newTemperature); // Reacting to difference accumulated in time (last 10 logs)
        double derivativeTermValue = calculateDerivativeTermValue(newTemperature); // Reacting to quickness of change
        double openingTermValue = calculateOpeningTermValue(newTemperature); // Boost opening if temperature above Optimal; or a bit if negative (below optimal) close to Optimal

        double newOpeningDiff = proportionalTermValue +
            integralTermValue +
            derivativeTermValue + 
            openingTermValue;

        // Avoid changing window opening if change from current one is less than provided threshold
        if (abs(newOpeningDiff) < CHANGE_DIFF_THRESHOLD) {
            newOpeningDiff = 0;
        }

        int newWindowOpening = limitFromExtremes(lastLog.windowOpening + newOpeningDiff);
        
        // Add new log to history
        addLog(newTemperature, newWindowOpening);

        return newWindowOpening;
    }
}