#include "persistence.h"


bool saveTargets() {
  StaticJsonDocument<4096> doc;
  JsonArray arr = doc.to<JsonArray>();

  auto addParam = [&](Scope scope, uint32_t zoneId, uint32_t flowerpotId, const Parameter& p){
    JsonObject o = arr.createNestedObject();
    o["scope"] = scopeToCode(scope);
    o["zoneId"] = zoneId;
    o["flowerpotId"] = flowerpotId;
    o["paramId"] = p.id;
    o["requestedValue"] = p.requestedValue;
  };

  // greenhouse-level
  for (auto &p : gh.parameters) addParam("greenhouse", 0, 0, p);
  // zones
  for (auto &z : gh.zones) {
    for (auto &p : z.parameters) addParam("zone", z.id, 0, p);
    for (auto &fp : z.flowerpots) {
      for (auto &p : fp.parameters) addParam("flowerpot", z.id, fp.id, p);
    }
  }

  File f = LittleFS.open("/targets.json", "w");
  if (!f) { Serial.println("[PERSIST] Failed to open /targets.json for write"); return false; }
  if (serializeJson(doc, f) == 0) { Serial.println("[PERSIST] Failed to write JSON"); f.close(); return false; }
  f.close();
  Serial.println("[PERSIST] Saved targets");
  return true;
}

bool loadTargets() {
  if (!LittleFS.exists("/targets.json")) {
    Serial.println("[PERSIST] /targets.json not found");
    return false;
  }
  File f = LittleFS.open("/targets.json", "r");
  if (!f) { Serial.println("[PERSIST] Failed to open /targets.json"); return false; }
  StaticJsonDocument<4096> doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) { Serial.printf("[PERSIST] Parse error: %s\n", err.c_str()); return false; }

  // Temporarily store to apply after greenhouse model is parsed
  // We'll keep them in memory for one-shot apply
  static std::vector<TargetRec> pendingTargets;
  pendingTargets.clear();

  for (JsonObject o : doc.as<JsonArray>()) {
    TargetRec r;
    r.scope = codeToScope((uint8_t)(o["scope"] | 0));
    r.zoneId = o["zoneId"] | 0;
    r.flowerpotId = o["flowerpotId"] | 0;
    r.paramId = o["paramId"] | 0;
    r.requestedValue = o["requestedValue"] | NAN;
    pendingTargets.push_back(r);
  }

  // Store pointer in a global lambda so we can apply after model load
  auto apply = [&]() {
    auto findAndApply = [&](const TargetRec& r) {
      if (r.scope == Scope::Greenhouse) {
        for (auto &p : gh.parameters) if (p.id == r.paramId) p.requestedValue = r.requestedValue;
      } else if (r.scope == Scope::Zone) {
        for (auto &z : gh.zones) if (z.id == r.zoneId)
          for (auto &p : z.parameters) if (p.id == r.paramId) p.requestedValue = r.requestedValue;
      } else if (r.scope == Scope::Flowerpot) {
        for (auto &z : gh.zones) if (z.id == r.zoneId)
          for (auto &fp : z.flowerpots) if (fp.id == r.flowerpotId)
            for (auto &p : fp.parameters) if (p.id == r.paramId) p.requestedValue = r.requestedValue;
      }
    };
    for (auto &r : pendingTargets) findAndApply(r);
    Serial.println("[PERSIST] Applied persisted targets to model");
  };

  // Make a global function pointer to call after model load
  static bool haveApply = false;
  static std::function<void(void)> applyFn;
  applyFn = apply;
  haveApply = true;

  // Expose an accessor to call once model is loaded
  applyPersistedTargetsToModel = [](){
    static bool done = false;
    if (done) return;
    done = true;
    if (haveApply) applyFn();
  };

  return true;
}

bool saveModelRaw(const char* buf, size_t len) {
File f=LittleFS.open("/model.json","w"); if(!f) return false;
size_t w=f.write((const uint8_t*)buf, len); f.close(); return w==len;
}
bool loadModelRaw(String& out) {
if(!LittleFS.exists("/model.json")) return false;
File f=LittleFS.open("/model.json","r"); if(!f) return false;
out = f.readString(); f.close(); return out.length()>0;
}
bool initFilesystem() {
    if (!LittleFS.begin()) {
        Serial.println("[ERROR] Failed to mount LittleFS");
        return false;
    }
    Serial.println("[FS] LittleFS mounted");
    return true;
}