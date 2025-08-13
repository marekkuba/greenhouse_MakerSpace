#include "config.h"
#include "sensor_types.h"
#include "sensors_if.h"
#include "device_types.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

void loadConfig() {
    if (!LittleFS.begin()) {
        Serial.println(F("[ERROR] LittleFS mount failed"));
        return;
    }
    if (!LittleFS.exists("/config.json")) {
        Serial.println(F("[ERROR] /config.json not found"));
        return;
    }

    File f = LittleFS.open("/config.json", "r");
    if (!f) {
        Serial.println(F("[ERROR] Failed to open config.json"));
        return;
    }

    // Read whole file into buffer
    size_t size = f.size();
    std::unique_ptr<char[]> buf(new char[size]);
    f.readBytes(buf.get(), size);
    f.close();

    // Parse JSON
    StaticJsonDocument<2048> doc;
    auto err = deserializeJson(doc, buf.get(), size);
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
        dev.flowerzoneId = obj["flowerzoneId"]| 0;
        dev.flowerId     = obj["flowerId"]    | 0;

        // Initialise actuators immediately
        if (dev.type == DeviceType::Toggle) {
            pinMode(dev.pin, OUTPUT);
            digitalWrite(dev.pin, LOW);
            Serial.printf("[CONFIG] Initialised toggle actuator on pin %d\n", dev.pin);
        }

        // Optional: pre-create sensors so they're ready before first controlTick()
        if (dev.driver != SensorDriver::Unknown && dev.type == DeviceType::Value) {
            float dummy;
            Sensors.read(dev.driver, dev.pin, "", dummy); // blank paramName if single output
            Serial.printf("[CONFIG] Pre-initialised %s sensor on pin %d\n",
                          String(driverName(dev.driver)).c_str(), dev.pin);
        }

        devices.push_back(dev);
    }

    Serial.printf("[CONFIG] Loaded %d devices from config.json\n", (int)devices.size());
}

void loadNetworkConfig() {
  if (!LittleFS.begin()) {
    Serial.println("[ERROR] LittleFS mount failed\n");
    return;
  }

  auto filename = "/network_config.json";

  if (!LittleFS.exists(filename)) {
    Serial.println("[ERROR] %s not found\n", filename);
    return;
  }

  File f = LittleFS.open(filename, "r");
  if (!f) {
    Serial.println("[ERROR] Failed to open %s\n", filename);
    return;
  }

  size_t size = f.size();
  char* buf = new char[size];
  f.readBytes(buf, size);
  f.close();

  StaticJsonDocument<2048> doc;
  auto err = deserializeJson(doc, buf, size);
  delete[] buf;  // Zwolnienie pamięci

  if (err) {
    Serial.printf("[ERROR] Config parse failed: %s\n", err.c_str());
    return;
  }

  JsonArray arr = doc.as<JsonArray>();
  netConfig.wifi_ssid = doc["wifi_ssid"].as<String>();
  netConfig.wifi_password = doc["wifi_password"].as<String>();
  String mqttHostStr = doc["mqtt_host"].as<String>();
  netConfig.mqtt_port = doc["mqtt_port"] | 1883; // default 1883

  int parts[4] = {0};
  sscanf(mqttHostStr.c_str(), "%d.%d.%d.%d", &parts[0], &parts[1], &parts[2], &parts[3]);
  netConfig.mqtt_host = IPAddress(parts[0], parts[1], parts[2], parts[3]);
  Serial.printf("[CONFIG] Loaded connection config\n");
}
