#ifndef GLOBALS_H
#define GLOBALS_H

#include <vector>
#include <functional>

// Forward includes for the types these globals use.
// Adjust these includes if the types live elsewhere.
#include "config.h"
#include "model.h"
#include "mappings.h"
#include "persistence.h"
#include "mqtt.h"
#include "network.h"
#include "sensors_if.h"

// Declarations (no definitions/initializers here).
extern SensorManager Sensors;                                // defined in globals.cpp [web:2][web:21]
extern NetworkConfig netConfig;                              // defined in globals.cpp [web:2][web:21]
extern Greenhouse greenhouse;                                // defined in globals.cpp [web:2][web:21]
extern std::vector<ParamBinding> bindings;                   // defined in globals.cpp [web:2][web:21]
extern std::vector<DeviceConfig> devices;                    // defined in globals.cpp [web:2][web:21]

// Will be assigned a lambda in globals.cpp
extern std::function<void(void)> applyPersistedTargetsToModel; // defined in globals.cpp [web:2][web:21]

// From WiFi/MQTT libs
extern WiFiEventHandler wifiConnectHandler;                  // defined in globals.cpp [web:2][web:21]
extern WiFiEventHandler wifiDisconnectHandler;               // defined in globals.cpp [web:2][web:21]
extern AsyncMqttClient mqttClient;                           // defined in globals.cpp [web:2][web:21]
extern Ticker wifiReconnectTimer;                            // defined in globals.cpp [web:2][web:21]
extern Ticker mqttReconnectTimer;                            // defined in globals.cpp [web:2][web:21]

// Timing
extern const unsigned long PUBLISH_INTERVAL;                 // defined in globals.cpp [web:2][web:22]
extern unsigned long previousMillis;                         // defined in globals.cpp [web:2][web:22]

#endif // GLOBALS_H
