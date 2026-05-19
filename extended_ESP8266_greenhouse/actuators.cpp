#include "actuators.h"
#include <Arduino.h>
#include "sensor_types.h"
#include <map>

static std::map<uint8_t, bool> gPinInit;

void writeActuator(SensorDriver driver, uint8_t pin, bool level) {
   Serial.printf("[WRITE] digital -> LEVEL: %s PIN: %d \n", level ? "high":"low", pin);
    switch (driver) {
        case SensorDriver::Digital: {
            if (!gPinInit[pin]) {
                pinMode(pin, OUTPUT);
                gPinInit[pin] = true;
            }
            digitalWrite(pin, level ? HIGH : LOW);
            break;
        }
        default:
            Serial.printf("[WARN] writeActuator: unsupported driver %d on pin %u\n", (int)driver, pin);
            break;
    }
}
void writeActuatorPWM(SensorDriver driver, uint8_t pin, float dutyPercent) {
    switch (driver) {
        case SensorDriver::Digital: {
            if (!gPinInit[pin]) {
                pinMode(pin, OUTPUT);
                gPinInit[pin] = true;
            }

            // Constrain percentage 0.0 - 100.0
            if (dutyPercent < 0.0f) dutyPercent = 0.0f;
            if (dutyPercent > 100.0f) dutyPercent = 100.0f;

            // Map 0-100% to 0-1023 (ESP8266 default PWM range)
            int duty = (int)((dutyPercent / 100.0f) * 1023);
            Serial.printf("[WRITE] PWM ->  VALUE:%d, PIN:%d \n", duty, pin);
            analogWrite(pin, duty);
            break;
        }
        default:
            Serial.printf("[WARN] writeActuatorPWM: unsupported driver %d on pin %u\n", (int)driver, pin);
            break;
    }
}