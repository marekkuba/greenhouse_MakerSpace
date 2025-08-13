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
};
void loadConfig();
void loadNetworkConfig();

extern NetworkConfig netConfig;
struct DeviceConfig {
  String   name;
  SensorDriver   driver;
  DeviceType type;
  uint8_t  pin;
  float    minValue;
  float    maxValue;
  uint8_t  flowerzoneId;
  uint8_t  flowerId;
};

extern std::vector<DeviceConfig> devices;