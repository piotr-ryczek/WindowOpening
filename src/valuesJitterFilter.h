#ifndef VALUE_JITTER_FILTER_H
#define VALUE_JITTER_FILTER_H

#include <Arduino.h>
#include <unordered_map>

using namespace std;

struct JitterValue {
    int currentValue;
    int minValue;
    int maxValue;
    float percentageJitterThreshold;
    int valueChangeThreshold;
    unsigned long lastChangeTime;
};

class ValuesJitterFilter {
    public:
        ValuesJitterFilter();
        void addValue(const char* label, int initialValue, int minValue, int maxValue, float percentageJitterThreshold);
        void removeValue(const char* label);
        int updateValue(const char* label, int value);
    private:
        unordered_map<const char*, JitterValue> values;
};

#endif