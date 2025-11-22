#include <LittleFS.h>

#include "config.h"
#include "sensor_types.h"
#include "sensors_if.h"
#include "device_types.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "mappings.h"
#include "persistence.h"

void loadAllConfigs() {
    loadConfig();
    loadNetworkConfig();
    loadMappings();
    loadTargets();
}

void loadConfig() {
    std::string filename = "/config.json";
    Serial.printf("[INFO] trying to parse %s \n", filename.c_str());
    if (!LittleFS.exists(filename.c_str())) {
        Serial.printf("[ERROR] %s not found \n", filename.c_str());
        return;
    }

    File f = LittleFS.open(filename.c_str(), "r");
    if (!f) {
        Serial.printf("[ERROR] Failed to open %s \n", filename.c_str());
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err) {
        Serial.printf("[ERROR] Config parse failed: %s\n", err.c_str());
        return;
    }

    devices.clear();

    for (JsonObject obj : doc.as<JsonArray>()) {

        DeviceConfig dev;
        dev.name         = obj["name"]        | "";
        dev.driver       = parseSensorDriver(obj["driver"].as<String>());
        dev.type         = parseDeviceType(obj["type"].as<String>());
        dev.pin          = obj["pin"]         | 0;
        dev.minValue     = obj.containsKey("minValue")    ? obj["minValue"].as<float>()  : NAN;
        dev.maxValue     = obj.containsKey("maxValue")    ? obj["maxValue"].as<float>()  : NAN;
        Serial.printf("[CONFIG] Loaded %s\n", dev.name.c_str());

        // Initialise actuators immediately
         if (dev.driver != SensorDriver::Unknown && dev.type == DeviceType::Value) {
             float dummy;
             Sensors.read(dev.driver, dev.pin, "", dummy);
             Serial.printf("[CONFIG] Pre-initialised %s sensor on pin %u\n",
                           String(driverName(dev.driver)).c_str(), dev.pin);
         }
         devices.push_back(dev);
    }

    Serial.printf("[CONFIG] Loaded %d devices from config.json\n", (int)devices.size());
}
void loadNetworkConfig() {
  const char* filename = "/network_config.json";
  if (!LittleFS.exists(filename)) {
  Serial.printf("[ERROR] %s not found\n", filename);
   return;
  }
   File f = LittleFS.open(filename, "r");
   if (!f) {
    Serial.printf("[ERROR] Failed to open %s\n", filename);
    return;
   }

  // FIX: strumieniowo [6]
  JsonDocument doc;
  auto err = deserializeJson(doc, f);
  f.close();
  if (err) {
  Serial.printf("[ERROR] Config parse failed: %s\n", err.c_str());
  return;
   }


  netConfig.wifi_ssid     = doc["wifi_ssid"] | "";
  netConfig.wifi_password = doc["wifi_password"] | "";
  String mqttHostStr      = doc["mqtt_host"] | "";
  netConfig.mqtt_port     = doc["mqtt_port"] | 1883;

  int a=0,b=0,c=0,d=0;
  if (sscanf(mqttHostStr.c_str(), "%d.%d.%d.%d", &a,&b,&c,&d) == 4
      && a>=0 && a<=255 && b>=0 && b<=255 && c>=0 && c<=255 && d>=0 && d<=255) {
    netConfig.mqtt_host = IPAddress((uint8_t)a,(uint8_t)b,(uint8_t)c,(uint8_t)d);
  } else {
    Serial.println("[CONFIG] WARN: mqtt_host not IPv4 literal; attempt DNS");
  }
  Serial.printf("[CONFIG] Loaded connection config for SSID '%s'\n", netConfig.wifi_ssid.c_str());
}