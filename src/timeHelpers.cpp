#include <timeHelpers.h>
#include <ctime>
#include <sstream>
#include <iomanip>

using namespace std;

long int getSecondFromDateString(String date) {
    char dateBuffer[64];
    date.toCharArray(dateBuffer, sizeof(dateBuffer));

    istringstream date_stream(dateBuffer);
    struct tm date_c;
    date_stream >> get_time(&date_c, "%Y-%m-%d %H:%M:%S");

    return mktime(&date_c);
}

String getCurrentTime() {
    char buffer[64];
    struct tm currentTime;

    if (!getLocalTime(&currentTime)) {
        Serial.println("Failed to obtain current time");
        return "";
    }

    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &currentTime);

    return String(buffer);
}

float calculateHoursAhead(String dateToCompare) {
    String currentTime = getCurrentTime();

    long int currentTimeSeconds = getSecondFromDateString(currentTime);
    long int dateToCompareSeconds = getSecondFromDateString(dateToCompare);

    float secondsDiff = dateToCompareSeconds - currentTimeSeconds;

    return secondsDiff / 60 / 60; // Hours
}
