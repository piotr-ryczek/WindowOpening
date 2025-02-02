#ifndef TIME_HELPERs_H
#define TIME_HELPERs_H

#include <Arduino.h>
#include <time.h>

String getCurrentTime();
long int getSecondsFromDateString(String date);
float calculateHoursAhead(String dateToCompare); //From current time

#endif