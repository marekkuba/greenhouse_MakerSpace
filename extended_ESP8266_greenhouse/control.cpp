#include "control.h"
#include "config.h"
#include "sensors.h"
#include "mappings.h"
#include "actuators.h"
#include "model.h"
#include "mqtt.h"
void controlTick() {
    for (auto &b : bindings) {
        Parameter* p = findParameter(b);
        if (!p) continue;

        // 1. Read sensor → update model
        float val = NAN;
        if (Sensors.read(b.readDriver, b.readPin, p->name, val)) {
            p->currentValue = val; // model is up to date
        }

        // 2. Control logic
        if (p->mutableFlag && !isnan(p->requestedValue) && !isnan(p->currentValue)) {
            bool turnOn = false;
            float h = 0.5;

            if (b.control == "heat") {
                turnOn = (p->currentValue < p->requestedValue - h);
            } else if (b.control == "cool") {
                turnOn = (p->currentValue > p->requestedValue + h);
            } else if (b.control == "humidify") {
                turnOn = (p->currentValue < p->requestedValue - h);
            } else if (b.control == "dehumidify") {
                turnOn = (p->currentValue > p->requestedValue + h);
            }

            writeActuator(b.writeDriver, b.writePin, turnOn);
            p->actuatorState = turnOn;
        }
    }
}
