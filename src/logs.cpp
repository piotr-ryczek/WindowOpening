#include <logs.h>
#include <timeHelpers.h>

vector<Log> logs;

void addLog(double temperature, int windowOpening, int deltaTemporaryWindowOpening) {
    Serial.println("Adding log to history");
    Log newLog;

    String currentTime = getCurrentTime();

    if (currentTime == "") {
        Serial.println("Aborting adding log - current time is empty");
        return;
    }

    Serial.print("Current time: ");
    Serial.println(currentTime);

    newLog.date = currentTime;
    newLog.temperature = temperature;
    newLog.windowOpening = windowOpening;
    newLog.deltaTemporaryWindowOpening = deltaTemporaryWindowOpening;

    Serial.println("Adding log: ");
    Serial.print("Date: ");
    Serial.println(newLog.date);
    Serial.print("Temperature: ");
    Serial.println(newLog.temperature);
    Serial.print("Window Opening: ");
    Serial.println(newLog.windowOpening);
    Serial.print("Delta Temporary Window Opening: ");
    Serial.println(newLog.deltaTemporaryWindowOpening);

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