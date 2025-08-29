#include "control.h"
#include "config.h"
#include "sensors.h"
#include "mappings.h"
#include "actuators.h"
#include "model.h"
#include "mqtt.h"

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
        Parameter* p = findParameter(b);
        if (!p) continue;

        // 1. Read sensor → update model
        float val = NAN;
        if (Sensors.read(b.readDriver, b.readPin, p->name, val)) {
            p->currentValue = val; // model is up to date
        }

        // 2. Control logic
  if (!(p->mutableFlag && !isnan(p->requestedValue) && !isnan(p->currentValue))) continue;

  const float err = p->requestedValue - p->currentValue;
  const float h   = b.hysteresis;
  const uint16_t key = actKey(b.writeDriver, b.writePin);
  auto &st = gAct[key];
  const uint32_t now = millis();

  if (b.outputMode == OutputMode::Binary) {
    bool wantOn = st.on; // hold inside deadband
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
