#ifndef WINDOW_OPENING_CALCULATIONS_H
#define WINDOW_OPENING_CALCULATIONS_H

#include <tuple>
#include <backendApp.h>

namespace PIDController {
    std::tuple<int, BackendAppLog> calculateWindowOpening(double newTemperature);
}

#endif