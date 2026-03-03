#include "mappings.h"
#include "sensor_types.h"
#include "globals.h"



Scope stringToScope(const String& text) {
    if (text.equalsIgnoreCase("greenhouse")) return Scope::Greenhouse;
    if (text.equalsIgnoreCase("zone")) return Scope::Zone;
    if (text.equalsIgnoreCase("flowerpot")) return Scope::Flowerpot;
    return Scope::Greenhouse; // Default
}

bool loadMappings() {
  if (!LittleFS.exists("/mapping.json")) {
   Serial.println("[MAP] /mapping.json not found");
    return false;
    }
  File f = LittleFS.open("/mapping.json", "r");
  if (!f) {
   Serial.println("[MAP] open failed");
   return false;
   }
  DynamicJsonDocument doc(4096);
  auto err = deserializeJson(doc, f);
  f.close();
  if (err) {
   Serial.printf("[MAP] parse error: %s\n", err.c_str());
    return false;
     }

  bindings.clear();
  for (JsonObject o : doc.as<JsonArray>()) {
    ParamBinding b;
    b.scope = stringToScope(o["scope"].as<String>());
    b.zoneId = o["zoneId"] | 0;
    b.flowerpotId = o["flowerpotId"] | 0;
    b.paramName = o["paramName"] | "";
    b.readDriver = parseSensorDriver(o["readDriver"].as<String>());
    b.readPin = o["readPin"] | NO_PIN;
    b.muxChannel = o["muxChannel"] | 0;
    if (o.containsKey("muxSelPins")) {
        b.muxSelPins.clear();
        for (JsonVariant v : o["muxSelPins"].as<JsonArray>()) {
            b.muxSelPins.push_back(v.as<uint8_t>());
        }
    }
    b.writeDriver = parseSensorDriver(o["writeDriver"].as<String>());
    b.writePin = o["writePin"] | NO_PIN;
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
    for (auto &p : greenhouse.parameters) if (p.name == b.paramName) return &p;
  } else if (b.scope == Scope::Zone) {
    for (auto &z : greenhouse.zones) if (z.id == b.zoneId)
      for (auto &p : z.parameters) if (p.name == b.paramName) return &p;
  } else if (b.scope == Scope::Flowerpot) {
    for (auto &z : greenhouse.zones) if (z.id == b.zoneId)
      for (auto &fp : z.flowerpots) if (fp.id == b.flowerpotId)
        for (auto &p : fp.parameters) if (p.name == b.paramName) return &p;
  }
  return nullptr;
}
