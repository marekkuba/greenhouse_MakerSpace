#pragma once
#include <Arduino.h>
#include "sensor_types.h"

void writeActuator(SensorDriver driver, uint8_t pin, bool level);
// Future PWM (commented until implemented)
void writeActuatorPWM(SensorDriver driver, uint8_t pin, float dutyPercent);