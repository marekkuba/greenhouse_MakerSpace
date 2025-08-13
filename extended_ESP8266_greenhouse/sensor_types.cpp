#include "sensor_types.h"

SensorDriver parseSensorDriver(const String& text) {
    if (text.equalsIgnoreCase("analog"))  return SensorDriver::Analog;
    if (text.equalsIgnoreCase("digital")) return SensorDriver::Digital;
    if (text.equalsIgnoreCase("DHT22"))   return SensorDriver::DHT22;
    return SensorDriver::Unknown;
}

const __FlashStringHelper* driverName(SensorDriver d) {
    switch (d) {
        case SensorDriver::Analog:  return F("analog");
        case SensorDriver::Digital: return F("digital");
        case SensorDriver::DHT22:   return F("DHT22");
        default:                    return F("unknown");
    }
}
