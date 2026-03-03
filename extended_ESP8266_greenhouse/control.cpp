#include "control.h"
#include "sensors_if.h" // Assumes SensorManager 'Sensors' is available here
#include "actuators.h"  // Assumes writeActuator is available here
#include "globals.h"    // Access to global 'bindings' vector and 'greenhouse'

// The global list of optimized bindings
std::vector<RuntimeBinding> activeBindings;

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Internal state tracking for actuators (to handle minOn/minOff)
// Key: (Driver << 8) | Pin
struct ActState {
    bool on = false;
    uint32_t lastChangeMs = 0;
};
static std::map<uint16_t, ActState> gAct;

// Helper to generate unique key for the state map
inline uint16_t makeActKey(SensorDriver d, uint8_t pin) {
    return (uint16_t(d) << 8) | pin;
}

// ---------------------------------------------------------
// 1. RESOLUTION PHASE
// ---------------------------------------------------------
void resolveBindings() {
    // 1. Backup the old actuator states before clearing
    std::map<uint16_t, ActState> oldActStates = gAct;

    activeBindings.clear();
    gAct.clear();

    Serial.println(F("[CTRL] Resolving Bindings..."));

    for (const auto &b : bindings) {
        Parameter* p = findParameter(b);

        if (p) {
            RuntimeBinding rb;
            rb.config = b;
            rb.target = p;
            activeBindings.push_back(rb);

            // 2. Restore state if this actuator existed previously
            // Calculate the key for this specific driver/pin combo
            if (b.writePin != NO_PIN) {
                uint16_t key = makeActKey(b.writeDriver, b.writePin);

                // If we had state for this pin, restore it so we don't lose minOn/minOff timers
                if (oldActStates.count(key)) {
                    gAct[key] = oldActStates[key];
                }
            }
        } else {
            Serial.printf("[WARN] Orphan Binding: Param '%s' (Z:%d P:%d) not found in Model.\n",
                          b.paramName.c_str(), b.zoneId, b.flowerpotId);
        }
    }

    Serial.printf("[CTRL] Resolution complete. %u active hardware links optimized.\n", activeBindings.size());
}

// ---------------------------------------------------------
// 2. SENSING PHASE
// ---------------------------------------------------------
void readSensors() {
    // Iterate only through bindings that have been resolved
    for (auto &rb : activeBindings) {
        ParamBinding &b = rb.config;
        Parameter* p = rb.target; // INSTANT ACCESS - No searching

        // strict check: if no sensor assigned, skip
        if (b.readPin == NO_PIN) continue;

        float val = NAN;

        yield(); /// to give some breathing space for wifi etc.
        // Use SensorManager to read hardware
        // Note: SensorManager handles its own caching (e.g. DHT22 2-sec interval)
        bool success = Sensors.read(
            b.readDriver,
            b.readPin,
            p->name,
            val,
            b.muxChannel,
            b.muxSelPins
        );

        if (success) {
            if (b.mapInMin != b.mapInMax) {
                val = mapFloat(val, b.mapInMin, b.mapInMax, b.mapOutMin, b.mapOutMax);
            }
            p->currentValue = val;
        }
    }
}

// ---------------------------------------------------------
// 3. ACTUATION PHASE
// ---------------------------------------------------------

// Internal helper to apply Hysteresis/Timing logic and write to hardware
static void applyLogicAndWrite(ParamBinding &b, Parameter* p, bool wantOn, uint32_t now) {
    const uint16_t key = makeActKey(b.writeDriver, b.writePin);
    ActState &st = gAct[key]; // Reference to static state

    // --- Anti-Short-Cycle Logic (MinOn / MinOff) ---
    if (wantOn != st.on) {
        const uint32_t elapsed = now - st.lastChangeMs;

        if (wantOn && b.minOffMs > 0 && elapsed < b.minOffMs) {
            // We want to turn ON, but we haven't been OFF long enough
            wantOn = false;
        }
        else if (!wantOn && b.minOnMs > 0 && elapsed < b.minOnMs) {
            // We want to turn OFF, but we haven't been ON long enough
            wantOn = true;
        }
        else {
            // State change allowed
            st.on = wantOn;
            st.lastChangeMs = now;
        }
    }
    // If state didn't change, we keep st.on as is.

    // --- Hardware Write ---
    // Handle Active Low logic (if activeLow=true, ON means logic LOW)
    bool physicalLevel = b.activeLow ? !st.on : st.on;

    writeActuator(b.writeDriver, b.writePin, physicalLevel);

    // --- Model Feedback ---
    // Update the model so the UI knows if the heater is actually running
    p->actuatorState = st.on;

    // Special case: For Toggle buttons, the actuator state IS the value
    if (b.readPin == NO_PIN) {
        p->currentValue = st.on ? 1.0 : 0.0;
    }
}
static void applyLogicAndWritePWM(ParamBinding &b, Parameter* p, float targetValue) {
    // Basic write
    writeActuatorPWM(b.writeDriver, b.writePin, targetValue);

    // Update model state
    // For PWM, "actuatorState" (bool) is true if value > 0
    p->actuatorState = (targetValue > 1.0f);

    // For manual control (ReadPin == NO_PIN), the currentValue IS the requestedValue
    if (b.readPin == NO_PIN) {
        p->currentValue = targetValue;
    }
}
void runControlLogic() {
    uint32_t now = millis();

    for (auto &rb : activeBindings) {
        ParamBinding &b = rb.config;
        Parameter* p = rb.target; // INSTANT ACCESS

        // Strict check: if no actuator assigned, skip
        if (b.writePin == NO_PIN) continue;

        // Strict check: if parameter is immutable or has no target, skip
        if (!p->mutableFlag) continue;
        if (isnan(p->requestedValue)) continue;
        if (b.outputMode == OutputMode::PWM) {
            float targetPWM = 0.0f;

            // Case A: Manual Control (No sensor feedback)
            // The requestedValue IS the speed (0-100%)
            if (b.readPin == NO_PIN) {
                targetPWM = p->requestedValue;
            }
            // Case B: Proportional Control (Sensor feedback exists)
            // Example: As Temp exceeds target, Fan speeds up
            else if (!isnan(p->currentValue)) {
                float err = p->currentValue - p->requestedValue;
                // Simple Proportional Gain (K=10 for example)
                // If Temp is 2 degrees over, Fan = 2 * 10 = 20%
                // You might want to make Gain configurable later, for now hardcode or use simple scaling
                float gain = 10.0f;

                if (b.direction == Direction::Decrease) { // Cooling
                     if (err > 0) targetPWM = err * gain;
                } else if (b.direction == Direction::Increase) { // Heating
                     if (err < 0) targetPWM = -err * gain;
                }
            }

            applyLogicAndWritePWM(b, p, targetPWM);
            continue; // Skip the rest of the loop for this binding
        }
        // ---------------------------
        // LOGIC TYPE A: TOGGLE (Light, Valve)
        // ---------------------------
        if (p->parameterType.equalsIgnoreCase("TOGGLE")) {
            // Simple threshold: > 0.5 is ON
            bool wantOn = (p->requestedValue > 0.5);
            applyLogicAndWrite(b, p, wantOn, now);
            continue;
        }

        // ---------------------------
        // LOGIC TYPE B: REGULATION (Heater, Cooler, Humidifier)
        // ---------------------------

        // Safety: If we don't know the current value, DO NOT run logic
        if (isnan(p->currentValue)) continue;

        float err = p->requestedValue - p->currentValue;
        bool wantOn = p->actuatorState; // Default: Maintain current state (Deadband)

        if (b.direction == Direction::Increase) {
            // Example: HEATER
            // Target 25, Current 20 -> Err +5.
            // If Err > Hyst (e.g. 0.5), turn ON.
            if (err > b.hysteresis) {
                wantOn = true;
            }
            // Target 25, Current 26 -> Err -1.
            // If Err < -Hyst, turn OFF.
            else if (err < -b.hysteresis) {
                wantOn = false;
            }
        }
        else if (b.direction == Direction::Decrease) {
            // Example: COOLER
            // Target 20, Current 25 -> Err -5.
            // If Err < -Hyst, turn ON.
            if (err < -b.hysteresis) {
                wantOn = true;
            }
            // Target 20, Current 19 -> Err +1.
            // If Err > Hyst, turn OFF.
            else if (err > b.hysteresis) {
                wantOn = false;
            }
        }

        applyLogicAndWrite(b, p, wantOn, now);
    }
}