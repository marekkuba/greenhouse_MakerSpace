#include "config.h"
#include "sensor_types.h"
#include <LittleFS.h>

void loadConfig() {
  if (!LittleFS.begin()) {
    Serial.println("[ERROR] LittleFS mount failed");
    return;
  }

  if (!LittleFS.exists("/config.json")) {
    Serial.println("[ERROR] config.json not found");
    return;
  }

  File f = LittleFS.open("/config.json", "r");
  if (!f) {
    Serial.println("[ERROR] Failed to open config.json");
    return;
  }

  size_t size = f.size();
  // Dynamiczny bufor zamiast unique_ptr
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
  for (JsonObject obj : arr) {
    DeviceConfig dev;
    dev.name         = obj["name"].as<String>();
    dev.driver       = dev.driverEnum = parseSensorDriver(obj["driver"].as<String>());
    dev.type         = obj["type"].as<String>();
    dev.pin          = obj["pin"].as<uint8_t>();
    dev.minValue     = obj.containsKey("minValue") ? obj["minValue"].as<float>() : NAN;
    dev.maxValue     = obj.containsKey("maxValue") ? obj["maxValue"].as<float>() : NAN;
    dev.flowerzoneId = obj["flowerzoneId"].as<uint8_t>();
    dev.flowerId     = obj["flowerId"].as<uint8_t>();
    dev.dhtPtr       = nullptr;

    if (dev.driver == "DHT22") {
      // Sprawdź czy sensor już istnieje dla tego pinu
      bool sensorExists = false;
      for (auto &d : devices) {
        if (d.driver == "DHT22" && d.pin == dev.pin && d.dhtPtr) {
          dev.dhtPtr = d.dhtPtr;  // Użyj istniejącej instancji
          sensorExists = true;
          Serial.printf("[CONFIG] Reusing existing DHT22 on pin %d\n", dev.pin);
          break;
        }
      }

      // Jeśli nie istnieje, utwórz nowy
      if (!sensorExists) {
        DHT* sensor = new DHT(dev.pin, DHT22);
        sensor->begin();
        dev.dhtPtr = sensor;
        Serial.printf("[CONFIG] Initialized DHT22 on pin %d\n", dev.pin);
      }
    }

    if (dev.type == "toggle") {
      pinMode(dev.pin, OUTPUT);
      digitalWrite(dev.pin, LOW);
      Serial.printf("[CONFIG] Initialized toggle on pin %d\n", dev.pin);
    }

    devices.push_back(dev);
  }
  Serial.printf("[CONFIG] Loaded %d devices\n", (int)devices.size());
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
