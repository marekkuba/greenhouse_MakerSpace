#pragma once
#include <map>
#include <Arduino.h>

struct DHTReading {
  float temperature = NAN;
  float humidity = NAN;
  unsigned long lastReadTime = 0;
};

bool readDHT(uint8_t pin, DHT* sensor, float* temp, float* hum);
bool readValue(const String& driver, uint8_t pin, const String& paramName, float& out);

extern std::map<uint8_t, DHTReading> dhtCache;