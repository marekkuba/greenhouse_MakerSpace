#pragma once
#include <Arduino.h>

enum class DeviceType : uint8_t {
    Value,     // a sensor that produces numeric readings
    Toggle,    // an actuator with on/off state
    Unknown
};

inline DeviceType parseDeviceType(const String &text) {
    if (text.equalsIgnoreCase("value"))  return DeviceType::Value;
    if (text.equalsIgnoreCase("toggle")) return DeviceType::Toggle;
    return DeviceType::Unknown;
}

inline const __FlashStringHelper* deviceTypeToString(DeviceType t) {
    switch (t) {
        case DeviceType::Value:  return F("value");
        case DeviceType::Toggle: return F("toggle");
        default:                 return F("unknown");
    }
}