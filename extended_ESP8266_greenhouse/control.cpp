#include "control.h"
#include "config.h"
#include "sensors_if.h"
#include "mappings.h"
#include "actuators.h"
#include "model.h"
#include "mqtt.h"
#include "globals.h"

namespace {
    struct ActState {
        bool on = false;
        uint32_t lastChangeMs = 0;
        uint8_t duty = 0; // reserved for future PWM mode
    };
    inline uint16_t actKey(SensorDriver d, uint8_t pin) {
        return (uint16_t(pin) | (uint16_t(uint8_t(d)) << 8));
    }
    std::map<uint16_t, ActState> gAct;
}

void controlTick() {
    for (auto &b : bindings) {
      Serial.printf("[MAP] Binding: z:%u fp:%u name=%s \n",
                    b.zoneId, b.flowerpotId, b.paramName.c_str());
        Parameter* p = findParameter(b);

        if (!p) continue;
      Serial.printf("[MAP] Parameter: name:%s \n",
                    p->name.c_str());
        // 1. Read sensor → update model
        float val = NAN;
        if(b.readPin != -1){
            if (Sensors.read(b.readDriver, b.readPin, p->name, val, b.muxChannel, b.muxSelPins)) {
                p->currentValue = val;
                 // model is up to date
                 Serial.printf("[MAP] Current value updated: %.2f \n",
                                     p->currentValue);
            }
        }
        // 2. Control logic
  Serial.printf("Debug -1\n");
  if (!(p->mutableFlag && !isnan(p->requestedValue) && !isnan(p->currentValue))) continue;
  Serial.printf("Debug 0\n");
  const float err = p->requestedValue - p->currentValue;
  const float h   = b.hysteresis;
  Serial.printf("Debug 1\n");
  const uint16_t key = actKey(b.writeDriver, b.writePin);
  auto &st = gAct[key];
  const uint32_t now = millis();

  if (b.outputMode == OutputMode::Binary) {
    bool wantOn = st.on; // hold inside deadband
    Serial.printf("Debug 2\n");
    if (b.direction == Direction::Increase) {
        if (err >  h)      wantOn = true;
            else if (err < -h) wantOn = false;
        } else if (b.direction == Direction::Decrease) {
            if (err < -h)      wantOn = true;
            else if (err >  h) wantOn = false;
        } else {
            // Unknown direction: keep previous state
        }
        // Enforce minOn/minOff anti-chatter
        if (wantOn != st.on) {
            const uint32_t elapsed = now - st.lastChangeMs;
            if (wantOn && b.minOffMs && elapsed < b.minOffMs) {
                wantOn = st.on;
            } else if (!wantOn && b.minOnMs && elapsed < b.minOnMs) {
                wantOn = st.on;
            }
            if (wantOn != st.on) {
                st.on = wantOn;
                st.lastChangeMs = now;
            }
        }

        const bool level = b.activeLow ? !st.on : st.on;
        writeActuator(b.writeDriver, b.writePin, level);
        p->actuatorState = st.on;
    }
  else if (b.outputMode == OutputMode::PWM) {
        // TODO (future): proportional control
        // float absErr = fabs(err);
        // uint8_t duty = computeDuty(absErr); // map/clamp 0..255
        // if (b.activeLow) duty = 255 - duty;
        // st.duty = duty;
        // writeActuatorPWM(b.writeDriver, b.writePin, st.duty, b.activeLow);
        // p->actuatorState = (st.duty > 0);
    }
  }
}
