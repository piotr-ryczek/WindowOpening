#ifndef WINDOW_OPENING_CALCULATIONS_H
#define WINDOW_OPENING_CALCULATIONS_H

#include <tuple>
#include <backendApp.h>

namespace PIDController {
    // Memory
    const double DEFAULT_OPTIMAL_TEMPERATURE = 22;
    const double DEFAULT_CHANGE_DIFF_THRESHOLD = 20; // Don't change opening below this number to avoid to often window manipulation

    std::tuple<int, BackendAppLog> calculateWindowOpening(double newTemperature);
}

#endif