#pragma once
#include <Arduino.h>

enum class SensorDriver : uint8_t {
    Analog,
    Digital,
    DHT22,
    Unknown
};

SensorDriver parseSensorDriver(const String& text);
const __FlashStringHelper* driverName(SensorDriver d);