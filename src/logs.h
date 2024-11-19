#ifndef LOGS_H
#define LOGS_H

#include <vector>

using namespace std;

const int MAX_LOGS = 10;

struct Log {
    double temperature;
    int windowOpening;
    int deltaTemporaryWindowOpening;
};

extern vector<Log> logs;

void addLog(double temperature, int windowOpening, int deltaTemporaryWindowOpening);
vector<Log> getLastLogs(int amount);

#endif