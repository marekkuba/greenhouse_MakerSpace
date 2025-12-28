#include "actuators.h"
#include <Arduino.h>
#include "sensor_types.h"
#include <map>

static std::map<uint8_t, bool> gPinInit;

void writeActuator(SensorDriver driver, uint8_t pin, bool level) {
//    Serial.printf("Writing actuator, Level: %s PIN: %d \n", level ? "high":"low", pin);
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
            // Unsupported actuator backend
            break;
    }
}

/*
// Example PWM scaffold
void writeActuatorPWM(SensorDriver driver, uint8_t pin, uint8_t duty, bool activeLow) {
    switch (driver) {
        case SensorDriver::Digital:
        // ESP8266: analogWrite(pin, activeLow ? (255 - duty) : duty);
        // Ensure pin supports PWM and is configured.
        break;
        default:
        break;
    }
}
*/