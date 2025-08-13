#include "network.h"

void setupWifi() {
  Serial.println("[WIFI] Connecting to Wi-Fi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.printf("[WIFI] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
  Serial.println("[WIFI] Waiting 1s before MQTT connection...");
  delay(1000);
  connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("[WIFI] Disconnected!");
  mqttReconnectTimer.detach();
  wifiReconnectTimer.once(2, setupWifi);
}
void registerWifiHandlers() {
    wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
    wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);
}