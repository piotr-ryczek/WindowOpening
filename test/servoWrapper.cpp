#include <Arduino.h>
#include <unity.h>
#include <ArduinoFake.h>

#include <servoWrapper.h>
#include <ESP32Servo.h>
#include <memoryValue.h>

using namespace fakeit;

void test_translateFrom100ToDegrees() {
    // Servo* servoMock = ArduinoFakeMock(Servo);
    // Servo servoInstance;
}

void setup() {
    UNITY_BEGIN();

    // RUN_TEST(test_translateFrom100ToDegrees);

    UNITY_END();
}