#include "persistence.h"
#include "mappings.h"
#include "model.h"

void tryOfflineModelRestore() {
    String json;
    if (loadModelRaw(json)) {
        Serial.println(F("[BOOT] Loaded saved greenhouse model."));
        parseGreenhouseJson(json.c_str());
    } else {
        Serial.println(F("[BOOT] No saved model found. Waiting for Server..."));
    }
}


bool saveModelRaw(const char* jsonString) {
    File f = LittleFS.open("/model.json", "w");
    if (!f) return false;
    size_t written = f.print(jsonString);
    f.close();
    Serial.println("[FS] Model saved to flash.");
    return written > 0;
}

bool loadModelRaw(String& outJson) {
    if(!LittleFS.exists("/model.json")) return false;
    File f = LittleFS.open("/model.json", "r");
    if (!f) return false;
    outJson = f.readString();
    f.close();
    return outJson.length() > 0;
}

bool validateAndSave(const char* filename, const char* jsonString, size_t len) {

    // 1. Validation: Check if it is valid JSON before saving
    // We use a small dynamic document just to check syntax.
    // We don't need to keep the data, just see if it parses.
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, jsonString, DeserializationOption::NestingLimit(10));

    if (error) {
        Serial.printf("[FS] JSON Validation failed for %s: %s\n", filename, error.c_str());
        return false;
    }

    // 2. Save to File
    File f = LittleFS.open(filename, "w");
    if (!f) {
        Serial.printf("[FS] Failed to open %s for writing\n", filename);
        return false;
    }

    size_t written = f.write((const uint8_t*)jsonString, len);
    f.close();

    if (written != len) {
        Serial.printf("[FS] Write mismatch for %s. Expected %d, wrote %d\n", filename, len, written);
        return false;
    }

    Serial.printf("[FS] Successfully saved %s (%d bytes)\n", filename, written);
    return true;
}

bool saveConfigRaw(const char* jsonString, size_t len) {
    return validateAndSave("/config.json", jsonString, len);
}

bool saveMappingRaw(const char* jsonString, size_t len) {
    return validateAndSave("/mapping.json", jsonString, len);
}

bool initFilesystem() {
    if (!LittleFS.begin()) {
       Serial.println("[ERROR] Failed to mount LittleFS");
        return false;
    }
   Serial.println("[FS] LittleFS mounted");
    return true;
}
