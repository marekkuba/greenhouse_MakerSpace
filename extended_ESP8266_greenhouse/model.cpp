#include "model.h"
#include "mqtt.h"
#include "persistence.h"
#include "control.h"
#include <Arduino.h>
#include <WString.h>

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

//    Serial.printf("[Deserialize Param] Param name: z:%s requestedVal: %.2f \n", dst.name.c_str(), dst.requestedValue);
}
bool parseGreenhouseJson(const char* json, size_t len, bool saveToDisk) {

    // A. Persistence Layer Call
    // We only save if this came from the network (saveToDisk = true).
    // If we are booting up and loading FROM disk, we pass false to avoid loop.
    if (saveToDisk) {
        if(saveModelRaw(json, len)) {
            Serial.println("[MODEL] New configuration saved to flash.");
        } else {
            Serial.println("[MODEL] Error saving configuration!");
        }
    }
    // B. Memory Allocation (CRITICAL CHANGE)
        // Use DynamicJsonDocument for large JSONs to use Heap instead of Stack
        DynamicJsonDocument doc(10240); // 10KB Buffer (adjust as needed)

        DeserializationError err = deserializeJson(doc, json, len);
        if (err) {
            Serial.printf("[MODEL] JSON parse error: %s\n", err.c_str());
            return false;
        }
    // C. Updating Structs
        Greenhouse newGh;
        newGh.id        = doc["id"] | 0;
        newGh.name      = doc["name"] | "Greenhouse";
        newGh.location  = doc["location"] | "";
        newGh.ipAddress = doc["ipAddress"] | "";
        newGh.status    = doc["status"] | "";

        // Greenhouse-level parameters
        if (doc.containsKey("parameters")) {
            for (JsonObject p : doc["parameters"].as<JsonArray>()) {
                Parameter param;
                deserializeParameter(p, param);
                newGh.parameters.push_back(param);
            }
        }

        // Zones
        if (doc.containsKey("zones")) {
            for (JsonObject z : doc["zones"].as<JsonArray>()) {
                Zone zone;
                zone.id = z["id"] | 0;
                zone.name = z["name"] | "Zone";

                // Zone parameters
                if (z.containsKey("parameters")) {
                    for (JsonObject p : z["parameters"].as<JsonArray>()) {
                        Parameter param;
                        deserializeParameter(p, param);
                        zone.parameters.push_back(param);
                    }
                }

                // Flowerpots
                if (z.containsKey("flowerpots")) {
                    for (JsonObject fp : z["flowerpots"].as<JsonArray>()) {
                        Flowerpot pot;
                        pot.id = fp["id"] | 0;
                        pot.name = fp["name"] | "Pot";
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
// D. Atomic Swap
    // We built the object safely on the side. Now we move it to global.
    greenhouse = std::move(newGh);
    resolveBindings();
    Serial.println("[MODEL1] Internal structures updated.");
    return true;
}

void publishModel() {
    Serial.println("[MODEL] Trying to publish model");
    // 1. Allocate (Use Dynamic for safety on ESP32)
    DynamicJsonDocument doc(4096);

    // 2. Serialize (Only sending IDs and currentValues to save bandwidth)
    doc["id"] = greenhouse.id;

    JsonArray zones = doc.createNestedArray("zones");
    for (const auto &z : greenhouse.zones) {
        JsonObject zj = zones.createNestedObject();
        zj["id"] = z.id;

        // Zone Params
        JsonArray zparams = zj.createNestedArray("parameters");
        for (const auto &p : z.parameters) {
            JsonObject o = zparams.createNestedObject();
            o["id"]  = p.id;
            o["val"] = p.currentValue;
        }

        // Flowerpots
        JsonArray fps = zj.createNestedArray("flowerpots");
        for (const auto &fp : z.flowerpots) {
            JsonObject fpj = fps.createNestedObject();
            fpj["id"] = fp.id;

            JsonArray fpparams = fpj.createNestedArray("parameters");
            for (const auto &p : fp.parameters) {
                JsonObject o = fpparams.createNestedObject();
                o["id"]  = p.id;
                o["val"] = p.currentValue;
            }
        }
    }

    // 3. Convert to String
    String output;
    serializeJson(doc, output);

    // 4. Hand off to MQTT layer
    // The model says: "Here is my data, send it."
    publishTelemetryJson(output, greenhouse.ipAddress);

    ///TODO: zastanowic sie, czy powinnismy zapisywac ten model tutaj
//    saveModelRaw(output.c_str(), output.length());
}
void loadModel() {
    String json;
    if (loadModelRaw(json)) {
        Serial.println(F("[BOOT] Loaded saved greenhouse model."));
        parseGreenhouseJson(json.c_str(),json.length(), false);
    } else {
        Serial.println(F("[BOOT] No saved model found. Waiting for Server..."));
    }
}