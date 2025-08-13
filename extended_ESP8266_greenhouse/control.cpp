#include "control.h"
#include "config.h"
#include "sensors.h"
#include "mappings.h"
#include "actuators.h"
#include "model.h"
#include "mqtt.h"   // for mqttClient

void controlTick() {

    for (auto &b : bindings) {
      Parameter* p = findParameter(b);
      if (!p) continue;

      // Read current sensor value
      float val = NAN;
      if (Sensors.read(b.readDriver, b.readPin, p->name, val)) {
        p->currentValue = val;
        // publish current value
        String tcur = makeParamTopic(b, "current");
        char payload[16]; dtostrf(val, 4, 2, payload);
        mqttClient.publish(tcur.c_str(), 1, true, payload);
      }

      // Control only if mutable and requestedValue is valid
      if (p->mutableFlag && !isnan(p->requestedValue) && !isnan(p->currentValue)) {
        bool turnOn = false;
        float hysteresis = 0.5; // tune as needed

        if (b.control == "heat") {
          // heat if below target - hysteresis
          if (p->currentValue < (p->requestedValue - hysteresis)) turnOn = true;
          if (p->currentValue > (p->requestedValue + hysteresis)) turnOn = false;
        } else if (b.control == "cool") {
          if (p->currentValue > (p->requestedValue + hysteresis)) turnOn = true;
          if (p->currentValue < (p->requestedValue - hysteresis)) turnOn = false;
        } else if (b.control == "humidify") {
          if (p->currentValue < (p->requestedValue - hysteresis)) turnOn = true;
          if (p->currentValue > (p->requestedValue + hysteresis)) turnOn = false;
        } else if (b.control == "dehumidify") {
          if (p->currentValue > (p->requestedValue + hysteresis)) turnOn = true;
          if (p->currentValue < (p->requestedValue - hysteresis)) turnOn = false;
        }

        writeActuator(b.writeDriver, b.writePin, turnOn);
        // Optionally publish actuator state
        String tact = makeParamTopic(b, "actuator");
        mqttClient.publish(tact.c_str(), 1, true, turnOn ? "ON" : "OFF");
      }

      // Also publish requested value so server can verify
      if (!isnan(p->requestedValue)) {
        String treq = makeParamTopic(b, "requested");
        char payload[16]; dtostrf(p->requestedValue, 4, 2, payload);
        mqttClient.publish(treq.c_str(), 1, true, payload);
      }
    }
}