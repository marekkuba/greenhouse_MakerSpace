#include "mappings.h"
#include "sensor_types.h"
#include "globals.h"
#include "device_log.h"



Scope stringToScope(const String& text) {
    if (text.equalsIgnoreCase("greenhouse")) return Scope::Greenhouse;
    if (text.equalsIgnoreCase("zone")) return Scope::Zone;
    if (text.equalsIgnoreCase("flowerpot")) return Scope::Flowerpot;
    Serial.printf("[MAP] Unknown scope '%s' — defaulting to Greenhouse\n", text.c_str());
    return Scope::Greenhouse;
}

bool loadMappings() {
  if (!LittleFS.exists("/mapping.json")) {
   Serial.println("[MAP] /mapping.json not found");
    return false;
    }
  File f = LittleFS.open("/mapping.json", "r");
  if (!f) {
   deviceLog("ERROR", "mapping.open", "Could not open /mapping.json");
   return false;
   }
  DynamicJsonDocument doc(4096);
  auto err = deserializeJson(doc, f);
  f.close();
  if (err) {
   deviceLog("ERROR", "mapping.parse", String("Mapping JSON parse error: ") + err.c_str());
    return false;
     }

  bindings.clear();
  for (JsonObject o : doc.as<JsonArray>()) {
    ParamBinding b;
    b.scope = stringToScope(o["scope"].as<String>());
    b.zoneId = o["zoneId"] | 0;
    b.flowerpotId = o["flowerpotId"] | 0;
    b.paramName = o["paramName"] | "";

    // Resolve the read sensor from the device inventory by id. The device is
    // the single source of truth for driver + pin; an unknown/zero id leaves
    // the sensor disabled (readPin == NO_PIN), which the control loop skips.
    b.readDeviceId = o["readDeviceId"] | 0;
    const DeviceConfig* rd = b.readDeviceId == 0 ? nullptr : findDevice(b.readDeviceId);
    if (rd) {
        b.readDriver = rd->driver;
        b.readPin    = rd->pin;
    } else {
        b.readDriver = SensorDriver::Unknown;
        b.readPin    = NO_PIN;
        if (b.readDeviceId != 0) {
            Serial.printf("[MAP] Warning: read device #%u not found for %s; sensor disabled\n",
                          b.readDeviceId, b.paramName.c_str());
        }
    }

    b.muxChannel = o["muxChannel"] | 0;
    if (o.containsKey("muxSelPins")) {
        b.muxSelPins.clear();
        for (JsonVariant v : o["muxSelPins"].as<JsonArray>()) {
            b.muxSelPins.push_back(v.as<uint8_t>());
        }
    }

    // Resolve the write actuator from the device inventory by id. Same
    // contract: unknown/zero id leaves the actuator disabled (writePin ==
    // NO_PIN) so no GPIO is driven by mistake.
    b.writeDeviceId = o["writeDeviceId"] | 0;
    const DeviceConfig* wd = b.writeDeviceId == 0 ? nullptr : findDevice(b.writeDeviceId);
    if (wd) {
        b.writeDriver = wd->driver;
        b.writePin    = wd->pin;
    } else {
        b.writeDriver = SensorDriver::Unknown;
        b.writePin    = NO_PIN;
        if (b.writeDeviceId != 0) {
            Serial.printf("[MAP] Warning: write device #%u not found for %s; actuator disabled\n",
                          b.writeDeviceId, b.paramName.c_str());
        }
    }
    b.direction   = parseDirection(o["direction"].as<String>());
    b.hysteresis  = o["hysteresis"] | 0.5;
    b.activeLow   = o["activeLow"]  | false;
    b.minOnMs     = o["minOnMs"]    | 0;
    b.minOffMs    = o["minOffMs"]   | 0;
    b.outputMode  = parseOutputMode(o["outputMode"] | "binary");
    b.mapInMin  = o["mapInMin"] | 0.0f;
    b.mapInMax  = o["mapInMax"] | 0.0f;
    b.mapOutMin = o["mapOutMin"] | 0.0f;
    b.mapOutMax = o["mapOutMax"] | 0.0f;
    if (b.direction == Direction::Unknown) {
      Serial.printf("[MAP] Warning: unknown direction for %s; defaulting to increase\n", b.paramName.c_str());
      b.direction = Direction::Increase;
    }
    if (b.outputMode == OutputMode::Unknown) {
      Serial.printf("[MAP] Warning: unknown outputMode for %s; defaulting to binary\n", b.paramName.c_str());
      b.outputMode = OutputMode::Binary;
    }

    if (b.writeDriver == SensorDriver::Digital && b.writePin != NO_PIN) {
      pinMode(b.writePin, OUTPUT);
      // MODIFIED: Only do digital init if mode is Binary
      if (b.outputMode == OutputMode::Binary) {
          const bool offLevel = b.activeLow ? HIGH : LOW;
          digitalWrite(b.writePin, offLevel);
          Serial.printf("[MAP] Init actuator pin %u OFF (activeLow=%d)\n", b.writePin, b.activeLow);
      }
      // NEW: Init PWM pins to 0
      else if (b.outputMode == OutputMode::PWM) {
          analogWrite(b.writePin, 0);
          Serial.printf("[MAP] Init actuator pin %u PWM 0%%\n", b.writePin);
      }
    }

    bindings.push_back(b);
  }
  Serial.printf("[MAP] Loaded %d bindings\n", (int)bindings.size());
  return true;
}

Parameter* findParameter(const ParamBinding& b) {
  if (b.scope == Scope::Greenhouse) {
    for (auto &p : greenhouse.parameters) if (p.name.equalsIgnoreCase(b.paramName)) return &p;
  } else if (b.scope == Scope::Zone) {
    for (auto &z : greenhouse.zones) if (z.id == b.zoneId)
      for (auto &p : z.parameters) if (p.name.equalsIgnoreCase(b.paramName)) return &p;
  } else if (b.scope == Scope::Flowerpot) {
    for (auto &z : greenhouse.zones) if (z.id == b.zoneId)
      for (auto &fp : z.flowerpots) if (fp.id == b.flowerpotId)
        for (auto &p : fp.parameters) if (p.name.equalsIgnoreCase(b.paramName)) return &p;
  }
  return nullptr;
}
