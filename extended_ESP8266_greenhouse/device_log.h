#pragma once
#include <Arduino.h>

// Remote logging: mirror an issue to Serial AND publish it to the server over
// MQTT (topic greenhouse/{ip}/log) so it can be read without a serial monitor.
//
// Safe to call from anywhere and at any rate: the implementation (in mqtt.cpp)
// dedupes consecutive identical messages and rate-limits with a token bucket so
// a stuck error path can never flood the broker or exhaust the ESP8266 heap.
//
// level: "INFO" | "WARN" | "ERROR"   code: short stable tag, e.g. "model.parse"
void deviceLog(const char* level, const char* code, const String& msg);
