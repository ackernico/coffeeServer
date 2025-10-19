#pragma once
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

extern AsyncWebServer server;
extern AsyncWebSocket ws;

void configServer();
void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);