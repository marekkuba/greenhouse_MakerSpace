#include "model.h"

void deserializeParameter(const JsonObject& src, Parameter& dst) {
    dst.id = src["id"] | 0;
    dst.name = src["name"] | "";
    dst.mutableFlag = src["mutable"] | false;
    dst.currentValue = src["currentValue"] | NAN;
    dst.requestedValue= src["requestedValue"] | NAN;
    dst.min = src["min"] | NAN;
    dst.max = src["max"] | NAN;
    dst.unit = src["unit"] | "";
    dst.parameterType = src["parameterType"] | "";
}

bool parseGreenhouseJson(const char* json, size_t len) {
  StaticJsonDocument<16384> doc; // increase if your model grows
  DeserializationError err = deserializeJson(doc, json, len);
  if (err) {
    Serial.printf("[MODEL] JSON parse error: %s\n", err.c_str());
    return false;
  }

  Greenhouse newGh;
  newGh.id        = doc["id"] | 0;
  newGh.name      = doc["name"] | "";
  newGh.location  = doc["location"] | "";
  newGh.ipAddress = doc["ipAddress"] | "";
  newGh.status    = doc["status"] | "";

  // greenhouse-level parameters
  if (doc.containsKey("parameters")) {
    for (JsonObject p : doc["parameters"].as<JsonArray>()) {
      Parameter param;
      deserializeParameter(p, param);
      newGh.parameters.push_back(param);
    }
  }

  // zones
  if (doc.containsKey("zones")) {
    for (JsonObject z : doc["zones"].as<JsonArray>()) {
      Zone zone;
      zone.id = z["id"] | 0;
      zone.name = z["name"] | "";

      // zone parameters
      if (z.containsKey("parameters")) {
        for (JsonObject p : z["parameters"].as<JsonArray>()) {
          Parameter param;
          deserializeParameter(p, param);
          zone.parameters.push_back(param);
        }
      }

      // flowerpots
      if (z.containsKey("flowerpots")) {
        for (JsonObject fp : z["flowerpots"].as<JsonArray>()) {
          Flowerpot pot;
          pot.id = fp["id"] | 0;
          pot.name = fp["name"] | "";
          if (fp.containsKey("parameters")) {
            for (JsonObject p : fp["parameters"].as<JsonArray>()) {
              Parameter param;
              deserializeParameter(p, param);
              pot.parameters.push_back(param);
            }
          }
          zone.flowerpots.push_back(pot);
        }
      }

      newGh.zones.push_back(zone);
    }
  }

  greenhouse = std::move(newGh);
  Serial.println("[MODEL] Greenhouse model updated from JSON");

  applyPersistedTargetsToModel();
  saveTargets();
  return true;
}
