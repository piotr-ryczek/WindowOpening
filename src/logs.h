#ifndef LOGS_H
#define LOGS_H

#include <vector>

using namespace std;

const int MAX_LOGS = 10;

struct Log {
    double temperature;
    int windowOpening;
};

extern vector<Log> logs;

void addLog(double temperature, int windowOpening);
vector<Log> getLastLogs(int amount);

#endif