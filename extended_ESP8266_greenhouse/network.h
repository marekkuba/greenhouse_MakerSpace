#pragma once
#include <Ticker.h>
#include <ESP8266WiFi.h>
void setupWifi();
void onWifiConnect(const WiFiEventStationModeGotIP& event);
void onWifiDisconnect(const WiFiEventStationModeDisconnected& event);
void registerWifiHandlers();
extern WiFiEventHandler wifiConnectHandler;
extern WiFiEventHandler wifiDisconnectHandler;
extern Ticker wifiReconnectTimer;
extern Ticker mqttReconnectTimer;
