#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <vector>
#include "model.h"
#include <functional>

enum class Scope : uint8_t { Greenhouse = 0, Zone = 1, Flowerpot = 2 };
inline uint8_t scopeToCode(Scope s) {
  switch (s) { case Scope::Greenhouse: return 0; case Scope::Zone: return 1; case Scope::Flowerpot: return 2; }
  return 0;
}
inline Scope codeToScope(uint8_t code) {
  switch (code) { case 0: return Scope::Greenhouse; case 1: return Scope::Zone; case 2: return Scope::Flowerpot; }
  return Scope::Greenhouse;
}

bool saveModelRaw(const char* jsonString, size_t len);
bool loadModelRaw(String& out);
bool saveConfigRaw(const char* jsonString, size_t len);
bool saveMappingRaw(const char* jsonString, size_t len);
bool initFilesystem();

