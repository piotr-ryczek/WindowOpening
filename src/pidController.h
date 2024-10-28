#ifndef WINDOW_OPENING_CALCULATIONS_H
#define WINDOW_OPENING_CALCULATIONS_H

#include <tuple>
#include <backendApp.h>

namespace PIDController {
    // Memory
    const double DEFAULT_OPTIMAL_TEMPERATURE = 22;

    std::tuple<int, BackendAppLog> calculateWindowOpening(double newTemperature);
}

#endif