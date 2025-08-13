#pragma once
#include <ESP8266WiFi.h>
#include <Arduino.h>
#include <DHT.h>

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
  String   driver;
  String   type;
  uint8_t  pin;
  float    minValue;
  float    maxValue;
  uint8_t  flowerzoneId;
  uint8_t  flowerId;
  DHT*     dhtPtr;
};

extern std::vector<DeviceConfig> devices;