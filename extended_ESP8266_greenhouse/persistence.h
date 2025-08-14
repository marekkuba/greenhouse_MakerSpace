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
struct TargetRec { Scope scope; uint32_t zoneId; uint32_t flowerpotId; uint32_t paramId; float requestedValue; };
bool saveTargets();
bool loadTargets();
void applyPersistedTargetsToModel();
bool saveModelRaw(const char* buf, size_t len);
bool loadModelRaw(String& out);
extern std::function<void(void)> applyPersistedTargetsToModel;
bool initFilesystem();
void tryOfflineModelRestore();