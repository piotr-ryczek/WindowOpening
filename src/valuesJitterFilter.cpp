#include <valuesJitterFilter.h>

ValuesJitterFilter::ValuesJitterFilter() {}
void ValuesJitterFilter::addValue(const char* label, int initialValue, int minValue, int maxValue, float percentageJitterThreshold) {

    float percentageAsDecimal = percentageJitterThreshold / 100.0f;
    int valueChangeThreshold = round((abs(maxValue - minValue)) * percentageAsDecimal);

    Serial.println("PercentageJitterThreshold: " + String(percentageJitterThreshold));
    Serial.println("ValueChangeThreshold: " + String(valueChangeThreshold));

    this->values[label] = JitterValue{initialValue, minValue, maxValue, percentageJitterThreshold, valueChangeThreshold, millis()};
}

void ValuesJitterFilter::removeValue(const char* label) {
    this->values.erase(label);
}

int ValuesJitterFilter::updateValue(const char* label, int value) {
    auto it = this->values.find(label);
    if (it == this->values.end()) {
        throw std::runtime_error("There is no value with this label");
    }
    
    auto& jitterValue = it->second;

    if (millis() - jitterValue.lastChangeTime > 1000 || abs(value - jitterValue.currentValue) > jitterValue.valueChangeThreshold) {
        jitterValue.currentValue = value;
        jitterValue.lastChangeTime = millis();
    }


    return jitterValue.currentValue;
}