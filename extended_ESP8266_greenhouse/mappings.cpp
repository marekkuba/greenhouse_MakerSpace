#include "mappings.h"

bool loadMappings() {
  if (!LittleFS.exists("/mapping.json")) { Serial.println("[MAP] /mapping.json not found"); return false; }
  File f = LittleFS.open("/mapping.json", "r");
  if (!f) { Serial.println("[MAP] open failed"); return false; }
  StaticJsonDocument<4096> doc;
  auto err = deserializeJson(doc, f);
  f.close();
  if (err) { Serial.printf("[MAP] parse error: %s\n", err.c_str()); return false; }

  bindings.clear();
  for (JsonObject o : doc.as<JsonArray>()) {
    ParamBinding b;
    b.scope = o["scope"] | "";
    b.zoneId = o["zoneId"] | 0;
    b.flowerpotId = o["flowerpotId"] | 0;
    b.paramName = o["paramName"] | "";

    b.readDriver = o["readDriver"] | "";
    b.readPin = o["readPin"] | 0;
    b.writeDriver = o["writeDriver"] | "";
    b.writePin = o["writePin"] | 0;
    b.control = o["control"] | "";

    bindings.push_back(b);
  }
  Serial.printf("[MAP] Loaded %d bindings\n", (int)bindings.size());
  return true;
}

Parameter* findParameter(const ParamBinding& b) {
  if (b.scope == Scope::Greenhouse) {
    for (auto &p : gh.parameters) if (p.name == b.paramName) return &p;
  } else if (b.scope == Scope::Zone) {
    for (auto &z : gh.zones) if (z.id == b.zoneId)
      for (auto &p : z.parameters) if (p.name == b.paramName) return &p;
  } else if (b.scope == Scope::Flowerpot) {
    for (auto &z : gh.zones) if (z.id == b.zoneId)
      for (auto &fp : z.flowerpots) if (fp.id == b.flowerpotId)
        for (auto &p : fp.parameters) if (p.name == b.paramName) return &p;
  }
  return nullptr;
}

String makeParamTopic(const ParamBinding& b, const char* field /* "current" or "requested" */) {
  String t = "greenhouse";
  if (b.scope == "zone") {
    t += "/zone" + String(b.zoneId);
  } else if (b.scope == "flowerpot") {
    t += "/zone" + String(b.zoneId) + "/plant" + String(b.flowerpotId);
  }
  t += "/" + b.paramName + "/" + field;
  return t;
}
