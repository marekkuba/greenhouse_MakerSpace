#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <vector>
#include "model.h"
#include "sensor_types.h"

enum class OutputMode : uint8_t {
    Binary = 0, // on/off
    PWM = 1, // proportional (future)
    Unknown = 255
};

inline OutputMode parseOutputMode(const String& s) {
    if (s.equalsIgnoreCase("binary")) return OutputMode::Binary;
    if (s.equalsIgnoreCase("pwm")) return OutputMode::PWM;
    return OutputMode::Unknown;
}

inline const __FlashStringHelper* outputModeToString(OutputMode m) {
    switch (m) {
        case OutputMode::Binary: return F("binary");
        case OutputMode::PWM: return F("pwm");
        default: return F("unknown");
    }
}

enum class Direction : uint8_t {
    Increase = 0,
    Decrease = 1,
    Unknown = 255
};

inline Direction parseDirection(const String& s) {
    if (s.equalsIgnoreCase("increase")) return Direction::Increase;
    if (s.equalsIgnoreCase("decrease")) return Direction::Decrease;
    return Direction::Unknown;
}

inline const __FlashStringHelper* directionToString(Direction d) {
    switch (d) {
        case Direction::Increase: return F("increase");
        case Direction::Decrease: return F("decrease");
        default: return F("unknown");
    }
}

struct ParamBinding {
    Scope scope;
    uint32_t zoneId = 0;
    uint32_t flowerpotId = 0;
    String paramName;

    SensorDriver readDriver;
    uint8_t      readPin = 0;

    SensorDriver writeDriver;
    uint8_t      writePin = 0;
    Direction  direction = Direction::Increase; // enum
    float      hysteresis = 0.5f;
    bool       activeLow = false;
    uint32_t   minOnMs = 0;
    uint32_t   minOffMs = 0;
    OutputMode outputMode = OutputMode::Binary;
};

bool loadMappings();
Parameter* findParameter(const ParamBinding& b);
