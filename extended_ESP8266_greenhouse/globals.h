#ifndef GLOBALS_H
#define GLOBALS_H

#include <vector>
#include <functional>
#include <Arduino.h>
#include <Ticker.h>

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

// From WiFi/MQTT libs
extern WiFiEventHandler wifiConnectHandler;                  // defined in globals.cpp [web:2][web:21]
extern WiFiEventHandler wifiDisconnectHandler;               // defined in globals.cpp [web:2][web:21]
extern AsyncMqttClient mqttClient;                           // defined in globals.cpp [web:2][web:21]
extern Ticker wifiReconnectTimer;                            // defined in globals.cpp [web:2][web:21]
extern Ticker mqttReconnectTimer;                            // defined in globals.cpp [web:2][web:21]

// Timing
const unsigned long PUBLISH_INTERVAL = 10000;                 // defined in globals.cpp [web:2][web:22]
extern unsigned long previousMillis;                         // defined in globals.cpp [web:2][web:22]

/// rebooting
extern bool systemRebootNeeded;

extern String newModelMessage;
extern bool newModelMessageArrived;

extern String newConfigMessage;
extern bool newConfigMessageArrived;

extern String newBindingMessage;
extern bool newBindingMessageArrived;

const uint8_t NO_PIN = 255;
#endif // GLOBALS_H
