#pragma once

void writeActuator(SensorDriver driver, uint8_t pin, bool on);
// Future PWM (commented until implemented)
// void writeActuatorPWM(SensorDriver driver, uint8_t pin, uint8_t duty, bool activeLow);