#ifndef WINDOW_OPENING_CALCULATIONS_H
#define WINDOW_OPENING_CALCULATIONS_H

#include <tuple>
#include <backendApp.h>

namespace PIDController {
    // Memory
    const double DEFAULT_OPTIMAL_TEMPERATURE = 22;
    const double DEFAULT_CHANGE_DIFF_THRESHOLD = 20; // Don't change opening below this number to avoid to often window manipulation
    
    const double DEFAULT_P_TERM_POSITIVE = 25;
    const double DEFAULT_P_TERM_NEGATIVE = 15;
    const double DEFAULT_D_TERM_POSITIVE = 50;
    const double DEFAULT_D_TERM_NEGATIVE = 45;

    // Factor pushing to open when diff with Optimal Temperature is relatively small
    const double DEFAULT_O_TERM_POSITIVE = 5; // Opening Term
    const double DEFAULT_O_TERM_NEGATIVE = 0.2; // Opening Term

    const double DEFAULT_I_TERM = 4;

    const double DEFAULT_OPENING_TERM_POSITIVE_TEMPERATURE_INCREASE = 5;

    std::tuple<int, BackendAppLog> calculateWindowOpening(double newTemperature);
}

#endif