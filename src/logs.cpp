#include <logs.h>

vector<Log> logs;

void addLog(double temperature, int windowOpening, int deltaTemporaryWindowOpening) {
    Log newLog;
    newLog.temperature = temperature;
    newLog.windowOpening = windowOpening;
    newLog.deltaTemporaryWindowOpening = deltaTemporaryWindowOpening;

    logs.push_back(newLog);

    if (logs.size() > MAX_LOGS) {
        logs.erase(logs.begin());
    }
}

vector<Log> getLastLogs(int amount) {
    vector<Log> lastLogs;

    if (amount > logs.size()) {
        amount = logs.size();
    }

    for (auto it = logs.rbegin(); it != logs.rbegin() + amount; it++) {
        lastLogs.push_back(*it);
    }

    return lastLogs;
}