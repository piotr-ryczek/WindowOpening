#include <timeHelpers.h>
#include <ctime>
#include <sstream>
#include <iomanip>

using namespace std;

long int getSecondsFromDateString(String date) {
    char dateBuffer[64];
    date.toCharArray(dateBuffer, sizeof(dateBuffer));

    istringstream date_stream(dateBuffer);
    struct tm date_c;
    date_stream >> get_time(&date_c, "%Y-%m-%d %H:%M:%S");

    return mktime(&date_c);
}

String getCurrentTime() {
    char buffer[128];
    struct tm currentTime;

    if (!getLocalTime(&currentTime)) {
        Serial.println("Failed to obtain current time");
        return "";
    }

    if (currentTime.tm_year < 70) { 
        Serial.println("Invalid time data");
        return "";
    }

    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &currentTime);

    return String(buffer);
}

/**
 * How many hours ahead from now is provided date
 */
float calculateHoursAhead(String dateToCompare) {
    String currentTime = getCurrentTime();

    if (currentTime == "") {
        return false;
    }

    long int currentTimeSeconds = getSecondsFromDateString(currentTime);
    long int dateToCompareSeconds = getSecondsFromDateString(dateToCompare);

    float secondsDiff = dateToCompareSeconds - currentTimeSeconds;

    return secondsDiff / 60 / 60; // Hours
}
