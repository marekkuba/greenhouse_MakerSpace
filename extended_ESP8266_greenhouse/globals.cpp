#include "config.h"
#include "model.h"
#include "mappings.h"
#include "persistence.h"
#include "mqtt.h"
#include "network.h"
NetworkConfig netConfig;
Greenhouse greenhouse;
std::vector<ParamBinding> bindings;
std::map<uint8_t, DHTReading> dhtCache;
std::vector<DeviceConfig> devices;
// Provide a default no-op. Will be overridden in loadTargets() if file exists.

std::function<void(void)> applyPersistedTargetsToModel = [](){};
WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
AsyncMqttClient mqttClient;
Ticker wifiReconnectTimer;
Ticker mqttReconnectTimer;

const unsigned long PUBLISH_INTERVAL = 10000;
unsigned long previousMillis = 0;