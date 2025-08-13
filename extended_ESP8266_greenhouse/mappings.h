#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <vector>
#include "model.h"

struct ParamBinding {
  String scope;
  uint32_t zoneId = 0;
  uint32_t flowerpotId = 0;
  String paramName;

  String readDriver;
  uint8_t readPin = 0;

  String writeDriver;
  uint8_t writePin = 0;

  String control; // "heat", "cool", "humidify", "dehumidify"
};

bool loadMappings();
Parameter* findParameter(const ParamBinding& b);
String makeParamTopic(const ParamBinding& b, const char* field /* "current" or "requested" */);