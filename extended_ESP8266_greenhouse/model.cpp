#include "model.h"
#include "mqtt.h"
#include "persistence.h"
#include "control.h"
#include "globals.h"
#include "device_log.h"
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

    // B. Parse first — the static doc validates JSON without touching flash.
    // Static: allocated once at first call, reused on every subsequent call with clear().
    // Eliminates repeated 10 KB heap alloc/free that fragment the ESP8266 heap over time.
    static DynamicJsonDocument doc(10240);
    doc.clear();

    DeserializationError err = deserializeJson(doc, json, len);
    if (err) {
        deviceLog("ERROR", "model.parse", String("Model JSON parse error: ") + err.c_str());
        return false;
    }

    // A. Persistence — only save after successful parse so a corrupt payload never reaches flash.
    // If we are booting up and loading FROM disk, saveToDisk=false to avoid a write loop.
    if (saveToDisk) {
        if(saveModelRaw(json, len)) {
            Serial.println("[MODEL] New configuration saved to flash.");
        } else {
            deviceLog("ERROR", "model.save", "Failed to save model to flash");
        }
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
    // Static: allocated once, reused every 10 s with clear() to prevent heap fragmentation.
    static DynamicJsonDocument doc(4096);
    doc.clear();

    // 2. Serialize (Only sending IDs and currentValues to save bandwidth)
    doc["id"] = greenhouse.id;

    // Greenhouse-level parameters
    JsonArray ghparams = doc.createNestedArray("parameters");
    for (const auto &p : greenhouse.parameters) {
        JsonObject o = ghparams.createNestedObject();
        o["id"]  = p.id;
        o["name"] = p.name;
        o["val"] = p.currentValue;
        o["req"] = p.requestedValue;
    }

    JsonArray zones = doc.createNestedArray("zones");
    for (const auto &z : greenhouse.zones) {
        JsonObject zj = zones.createNestedObject();
        zj["id"] = z.id;

        JsonArray zparams = zj.createNestedArray("parameters");
        for (const auto &p : z.parameters) {
            JsonObject o = zparams.createNestedObject();
            o["id"]  = p.id;
            o["name"] = p.name;
            o["val"] = p.currentValue;
            o["req"] = p.requestedValue;
        }

        JsonArray fps = zj.createNestedArray("flowerpots");
        for (const auto &fp : z.flowerpots) {
            JsonObject fpj = fps.createNestedObject();
            fpj["id"] = fp.id;

            JsonArray fpparams = fpj.createNestedArray("parameters");
            for (const auto &p : fp.parameters) {
                JsonObject o = fpparams.createNestedObject();
                o["id"]  = p.id;
                o["name"] = p.name;
                o["val"] = p.currentValue;
                o["req"] = p.requestedValue;
            }
        }
    }

    // 3. Convert to String
    String output;
    serializeJson(doc, output);

    // 4. Hand off to MQTT layer
    // The model says: "Here is my data, send it."
    publishTelemetryJson(output, netConfig.device_ip);

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