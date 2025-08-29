#include "actuators.h"

void writeActuator(SensorDriver driver, uint8_t pin, bool level) {
    switch (driver) {
        case SensorDriver::Digital: {
            pinMode(pin, OUTPUT);
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