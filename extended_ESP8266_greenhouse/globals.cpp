#include "config.h"
#include "model.h"
#include "mappings.h"
#include "persistence.h"
#include "mqtt.h"
#include "network.h"
#include "sensors_if.h"
#include <Ticker.h>

SensorManager Sensors;
NetworkConfig netConfig;
Greenhouse greenhouse;
std::vector<ParamBinding> bindings;
std::vector<DeviceConfig> devices;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
AsyncMqttClient mqttClient;
Ticker wifiReconnectTimer;
Ticker mqttReconnectTimer;
bool systemRebootNeeded = false;
const char* newModelMessage;
int newModelMessageLen;
bool newModelMessageArrived = false;

const char* newConfigMessage;
int newConfigMessageLen;
bool newConfigMessageArrived = false;

const char* newBindingMessage;
int newBindingMessageLen;
bool newBindingMessageArrived = false;
//const unsigned long PUBLISH_INTERVAL = 10000;
unsigned long previousMillis = 0;
//const uint8_t NO_PIN = 255;
const unsigned long REBOOT_INTERVAL_MS = 24UL * 60UL * 60UL * 1000UL; // 24 Hours
unsigned long bootTime = 0;