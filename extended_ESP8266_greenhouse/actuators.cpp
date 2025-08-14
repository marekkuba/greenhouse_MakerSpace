#include "actuators.h"

void writeActuator(SensorDriver driver, uint8_t pin, bool on) {
    switch (driver) {
        case SensorDriver::Digital: // used as toggle
            pinMode(pin, OUTPUT);
            digitalWrite(pin, on ? HIGH : LOW);
            break;
        default:
        // extend later (PWM, I2C, etc.)
        break;
    }
}