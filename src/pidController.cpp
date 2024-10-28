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

        return result > 0 ? -result : 0; // Returning negative value as air pollution can be only negatively percepted factor
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

    void attachConfigData(BackendAppLog& backendAppLog) {
        backendAppLog.config.weatherLogNotOlderThanHours = WEATHER_LOG_NOT_OLDER_THAN_HOURS;
        backendAppLog.config.pm25Norm = PM_25_NORM;
        backendAppLog.config.pm10Norm = PM_10_NORM;
        backendAppLog.config.pm25Weight = PM_25_WEIGHT;
        backendAppLog.config.pm10Weight = PM_10_WEIGHT;
        backendAppLog.config.maxOutsideTemperatureDiffFromOptimal = MAX_OUTSIDE_TEMPERATURE_DIFF_FROM_OPTIMAL;
        backendAppLog.config.outsideTemperatureClosingThreshold = OUTSIDE_TEMPERATURE_CLOSING_THRESHOLD;
        backendAppLog.config.optimalTemperature = OPTIMAL_TEMPERATURE;
        backendAppLog.config.pTermPositive = P_TERM_POSITIVE;
        backendAppLog.config.pTermNegative = P_TERM_NEGATIVE;
        backendAppLog.config.dTermPositive = D_TERM_POSITIVE;
        backendAppLog.config.dTermNegative = D_TERM_NEGATIVE;
        backendAppLog.config.oTermPositive = O_TERM_POSITIVE;
        backendAppLog.config.oTermNegative = O_TERM_NEGATIVE;
        backendAppLog.config.iTerm = I_TERM;
        backendAppLog.config.openingTermPositiveTemperatureIncrease = OPENING_TERM_POSITIVE_TEMPERATURE_INCREASE;
        backendAppLog.config.changeDiffThreshold = CHANGE_DIFF_THRESHOLD;
    }

    tuple<int, BackendAppLog> calculateWindowOpening(double newTemperature) {
        // BackendApp Log
        BackendAppLog backendAppLog;
        backendAppLog.insideTemperature = newTemperature;

        backendAppLog.outsideTemperature = nullptr;
        backendAppLog.pm25 = nullptr;
        backendAppLog.pm10 = nullptr;

        attachConfigData(backendAppLog);

        // Retrieve last log to compare
        Log lastLog = logs.back();

        // Calculate all terms values
        double proportionalTermValue = calculateProportionalTermValue(newTemperature); // Reacting to difference size
        double integralTermValue = calculateIntegralTermValue(newTemperature); // Reacting to difference accumulated in time (last 10 logs)
        double derivativeTermValue = calculateDerivativeTermValue(newTemperature); // Reacting to quickness of change
        double openingTermValue = calculateOpeningTermValue(newTemperature); // Boost opening if temperature above Optimal; or a bit if negative (below optimal) close to Optimal

        // BackendApp Log
        backendAppLog.partialData.proportionalTermValue = proportionalTermValue;
        backendAppLog.partialData.integralTermValue = integralTermValue;
        backendAppLog.partialData.derivativeTermValue = derivativeTermValue;
        backendAppLog.partialData.openingTermValue = openingTermValue;
        backendAppLog.partialData.outsideTemperatureTermValue = nullptr;
        backendAppLog.partialData.airPollutionTermValue = nullptr;

        double newOpeningDiff = proportionalTermValue +
            integralTermValue +
            derivativeTermValue + 
            openingTermValue;

        WeatherLog* lastWeatherLog = getLastWeatherLogNotTooOld(WEATHER_LOG_NOT_OLDER_THAN_HOURS);

        if (lastWeatherLog != nullptr) {
            double outsideTemperatureTermValue = calculateOutsideTemperatureTermValue(lastWeatherLog->outsideTemperature);
            double airPollutionTermValue = calculateAirPollutionTermValue(lastWeatherLog->pm25, lastWeatherLog->pm10);

            newOpeningDiff += outsideTemperatureTermValue;
            newOpeningDiff += airPollutionTermValue;

            // BackendApp Log
            backendAppLog.partialData.outsideTemperatureTermValue = &outsideTemperatureTermValue;
            backendAppLog.partialData.airPollutionTermValue = &airPollutionTermValue;

            backendAppLog.outsideTemperature = &lastWeatherLog->outsideTemperature;
            backendAppLog.pm25 = &lastWeatherLog->pm25;
            backendAppLog.pm10 = &lastWeatherLog->pm10;
        }

        // BackendApp Log
        backendAppLog.deltaTemporaryWindowOpening = newOpeningDiff;

        // Avoid changing window opening if change from current one is less than provided threshold
        if (abs(newOpeningDiff) < CHANGE_DIFF_THRESHOLD) {
            newOpeningDiff = 0;
        }

        // BackendApp Log
        backendAppLog.deltaFinalWindowOpening = newOpeningDiff;

        int newWindowOpening = limitFromExtremes(lastLog.windowOpening + newOpeningDiff);

        // BackendApp Log
        backendAppLog.windowOpening = newWindowOpening;
        
        // Add new log to history
        addLog(newTemperature, newWindowOpening);

        return make_tuple(newWindowOpening, backendAppLog);
    }
}