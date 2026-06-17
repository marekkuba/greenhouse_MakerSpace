#pragma once
#include <ESP8266WiFi.h>
#include <Arduino.h>
#include <DHT.h>
#include "sensor_types.h"
#include "device_types.h"

struct NetworkConfig {
  String wifi_ssid;
  String wifi_password;
  IPAddress mqtt_host;
  uint16_t mqtt_port;
  String mqtt_username;
  String mqtt_password;
  String device_ip;
};

void loadAllConfigs();
void loadConfig();
void loadNetworkConfig();

extern NetworkConfig netConfig;
struct DeviceConfig {
  uint16_t id = 0;          // server-assigned stable id (0 = unset)
  String   name;
  SensorDriver   driver;
  DeviceType type;
  uint8_t  pin;
};

extern std::vector<DeviceConfig> devices;

// Look up a device in the inventory by its server-assigned id. Returns nullptr
// if no device with that id is configured.
const DeviceConfig* findDevice(uint16_t id);