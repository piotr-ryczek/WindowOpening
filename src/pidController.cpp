#include <iostream>
#include <vector>
#include <pidController.h>
#include <logs.h>
#include <weatherLogs.h>

using namespace std;
using namespace PIDController;

namespace PIDController {
    const double WEATHER_LOG_NOT_OLDER_THAN_HOURS = 3;

    const double PM_25_NORM = 15;
    const double PM_10_NORM = 45;
    const double PM_25_WEIGHT = 4; // PM2.5 is more harmful for health so we take it into account more harshly
    const double PM_10_WEIGHT = 1;

    const double MAX_OUTSIDE_TEMPERATURE_DIFF_FROM_OPTIMAL = 45; // If Optimal 22' -> Outisde MAX will be -23' (so then window delta will be OUTSIDE_TEMPERATURE_CLOSING_THRESHOLD)
    const double OUTSIDE_TEMPERATURE_CLOSING_THRESHOLD = -80;

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

    /**
     * Provided amount to be deducted from final result
     */
    double calculateAirPollutionTermValue(double pm25, double pm10) {
        double pm25NormPropotion = pm25 / PM_25_NORM;
        double pm10NormPropotion = pm10 / PM_10_NORM;

        double accomulatedPropotion = (pm25NormPropotion * PM_25_WEIGHT + pm10NormPropotion * PM_10_WEIGHT) / (PM_25_WEIGHT + PM_10_WEIGHT);

        double result = (accomulatedPropotion * accomulatedPropotion) - 1;

        return result > 0 ? result : 0;
    }

    double calculateOutsideTemperatureTermValue(double outsideTemperature) {
        if (outsideTemperature >= OPTIMAL_TEMPERATURE) {
            return 100; // Increase by 100 if outside temperature is equal or higher than optimal one
        }

        double diffFromOptimal = abs(OPTIMAL_TEMPERATURE - outsideTemperature);

        double delta = (OUTSIDE_TEMPERATURE_CLOSING_THRESHOLD * diffFromOptimal) / MAX_OUTSIDE_TEMPERATURE_DIFF_FROM_OPTIMAL;

        return delta;
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

        WeatherLog* lastWeatherLog = getLastWeatherLogNotTooOld(WEATHER_LOG_NOT_OLDER_THAN_HOURS);

        if (lastWeatherLog != nullptr) {
            double outsideTemperatureTermValue = calculateOutsideTemperatureTermValue(lastWeatherLog->outsideTemperature);
            double airPollutionTermDeductValue = calculateAirPollutionTermValue(lastWeatherLog->pm25, lastWeatherLog->pm10);

            newOpeningDiff += outsideTemperatureTermValue;
            newOpeningDiff -= airPollutionTermDeductValue; // Air Polution provides positive number as it can only acts as negative factor (when Air is clean then factor does not matter)
        }

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