#pragma once
#include <ArduinoJson.h>
#include <vector>
#include <Arduino.h>

struct Parameter {
  uint32_t id = 0;
  String name;
  bool mutableFlag = false;
  float currentValue = NAN;
  float requestedValue = NAN;
  float min = NAN;
  float max = NAN;
  String unit;
  String parameterType;
};

struct Flowerpot {
  uint32_t id = 0;
  String name;
  std::vector<Parameter> parameters;
};

struct Zone {
  uint32_t id = 0;
  String name;
  std::vector<Flowerpot> flowerpots;
  std::vector<Parameter> parameters;
};

struct Greenhouse {
  uint32_t id = 0;
  String name;
  String location;
  String ipAddress;
  String status;
  std::vector<Zone> zones;
  std::vector<Parameter> parameters;
};

void deserializeParameter(const JsonObject& src, Parameter& dst);
bool parseGreenhouseJson(const char* json, size_t len, Greenhouse& out);

extern Greenhouse greenhouse;
