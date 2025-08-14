#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <vector>
#include "model.h"
#include "sensor_types.h"

struct ParamBinding {
    Scope scope;
    uint32_t zoneId = 0;
    uint32_t flowerpotId = 0;
    String paramName;

    SensorDriver readDriver;
    uint8_t      readPin = 0;

    SensorDriver writeDriver;
    uint8_t      writePin = 0;

    String control; // or effect
};

bool loadMappings();
Parameter* findParameter(const ParamBinding& b);
