#include "actuators.h"

void writeActuator(const String& driver, uint8_t pin, bool on) {
  if (driver == "toggle") {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, on ? HIGH : LOW);
  }
}