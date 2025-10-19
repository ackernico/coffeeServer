#include "../inc/webserver.h"
#include "../inc/fileManager.h"
#include "../inc/globals.h"

#include <string.h>
#include <Arduino.h>

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.println("ESP - WebSocket connected!");
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.println("ESP - WebSocket disconnected!");
  }
}

void configServer()
{
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/html/index.html", String(), false);
  });
  
  // Partials files route
  server.serveStatic("/html", LittleFS, "/html/");
  server.serveStatic("/css", LittleFS, "/css/");
  server.serveStatic("/assets", LittleFS, "/assets/");

  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/script.js", "text/javascript");
  });
  // Font file route
  server.on("/Inter.woff2", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/Inter.woff2", "font/woff2");
  });
  
  server.on("/on", HTTP_POST, [](AsyncWebServerRequest *request){},
  NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) 
    {
      String body;
      for(size_t i=0 ; i<len ; i++)
      {
        body += (char)data[i];
      }
      Serial.println("Received message: " + body);

      StaticJsonDocument<200> doc;
      DeserializationError err = deserializeJson(doc, body);
      if(err)
      {
        request->send(400, "application/json", "{\"error\":\"invalidJSON\"}");
        return;
      }

      const char* status = doc["status"];
      const char* thickness = doc["thickness"];

      std::string sStatus = status;

      if(sStatus == "on") webPower = true;
      else if(sStatus == "off") webPower = false;

      String response;
      StaticJsonDocument<200> respDoc;
      respDoc["received"] = true;
      respDoc["status"] = status;
      respDoc["thickness"] = thickness;
      serializeJson(respDoc, response);

      request->send(200, "application/json", response);
    });

    server.on("/on", HTTP_OPTIONS, [](AsyncWebServerRequest *request)
    {
      AsyncWebServerResponse *response = request->beginResponse(204);
      response->addHeader("Access-Control-Allow-Origin", "*");
      response->addHeader("Access-Control-Allow-Methods", "POST, OPTIONS");
      response->addHeader("Access-Control-Allow-Headers", "Content-Type");
      request->send(response);
    });

    server.on("/alarms", HTTP_POST, [](AsyncWebServerRequest *request) { },
    NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      String body;
      body.reserve(len + 1);
      for (size_t i = 0; i < len; ++i) body += (char)data[i];

      DynamicJsonDocument incoming(512);
      if (deserializeJson(incoming, body)) {
        request->send(400, "application/json", "{\"error\":\"invalid json\"}");
        return;
      }

      int idx = incoming["index"] | -1;
      bool status = incoming["status"] | false;
      int timeH = incoming["timeH"] | -1;
      int timeM = incoming["timeM"] | -1;
      int thickness = incoming["thickness"] | -1;
      const char* name = incoming["name"] | "";
      String repeatS = incoming["repeatS"].as<String>();

      bool repeat[7] = {false, false, false, false, false, false, false};
      if (incoming.containsKey("repeat") && incoming["repeat"].is<JsonArray>()) {
          JsonArray repeatArr = incoming["repeat"].as<JsonArray>();
          for (size_t i = 0; i < 7 && i < repeatArr.size(); ++i) {
              repeat[i] = repeatArr[i].as<bool>();
          }
      }

      DynamicJsonDocument doc(4096);
      DeserializationError err2 = deserializeJson(doc, storeAlarm);
      if (err2) {
        doc.clear();
        doc.to<JsonArray>();
      }

      JsonArray arr = doc.as<JsonArray>();

      while ((int)arr.size() <= idx) {
        arr.createNestedObject();
      }

      JsonObject slot = arr[idx].as<JsonObject>();
      slot["received"] = true;
      slot["index"] = idx;
      slot["status"] = status;
      slot["timeH"] = timeH;
      slot["timeM"] = timeM;
      slot["name"] = name;
      slot["thickness"] = thickness;
      slot["repeatS"] = repeatS;

      JsonArray repeatArrOut = slot.createNestedArray("repeat");
      for (int i = 0; i < 7; i++) {
          repeatArrOut.add(repeat[i]);
      }

      Alarm newAlarm;
      newAlarm.index = idx;
      newAlarm.status = status;
      newAlarm.timeH = timeH;
      newAlarm.timeM = timeM;
      newAlarm.thickness = thickness;
      newAlarm.name = name;
      newAlarm.repeatS = repeatS;
      for(int i=0 ; i<7 ; i++) newAlarm.repeat[i] = repeat[i];

      alarms.push_back(newAlarm);

      String out;
      serializeJson(arr, out);
      storeAlarm = out;
      writeJson("/alarms.json", storeAlarm.c_str());
      readJson("/alarms.json");

      request->send(200, "application/json", storeAlarm);
    });

    server.on("/data", HTTP_POST, [](AsyncWebServerRequest *request){},
  NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) 
    {
      String body;
      for(size_t i=0 ; i<len ; i++)
      {
        body += (char)data[i];
      }
      Serial.println("Received message: " + body);

      StaticJsonDocument<200> doc;
      DeserializationError err = deserializeJson(doc, body);
      if(err)
      {
        request->send(400, "application/json", "{\"error\":\"invalidJSON\"}");
        return;
      }

      const char* duration = doc["duration"];
      const char* date = doc["date"];
      const char* thickness = doc["thickness"];

      String response;
      StaticJsonDocument<200> respDoc;
      respDoc["received"] = true;
      respDoc["duration"] = duration;
      respDoc["date"] = date;
      respDoc["thickness"] = thickness;
      float avg = 0;
      if (powerSamples > 0)
        avg = (float)powerSum / powerSamples;
      else
        avg = 0;
      respDoc["avgPower"] = avg;

      serializeJson(respDoc, response);

      request->send(200, "application/json", response);
    });

  server.on("/alarms", HTTP_GET, [](AsyncWebServerRequest *request) {
    readJson("/alarms.json");
    request->send(200, "application/json", storeAlarm);
  });

  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);

  server.begin();
}