#include "../inc/webserver.h"
#include "../inc/fileManager.h"
#include "../inc/globals.h"

#include <string.h>
#include <Arduino.h>

#define MAX_ALARMS 100

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

      std::string sStatus = status;

      if(sStatus == "on") webPower = true;
      else if(sStatus == "off") webPower = false;

      String response;
      StaticJsonDocument<200> respDoc;
      respDoc["received"] = true;
      respDoc["status"] = status;
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
    DeserializationError errIncoming = deserializeJson(incoming, body);
    if (errIncoming) {
      request->send(400, "application/json", "{\"error\":\"invalid json\"}");
      return;
    }

    int idx = incoming["index"] | -1;
    bool status = incoming["status"] | false;
    int timeH = incoming["timeH"] | -1;
    int timeM = incoming["timeM"] | -1;
    const char* name = incoming["name"] | "";
    String repeatS = incoming["repeatS"].as<String>();

    // validate index
    if (idx < 0 || idx >= MAX_ALARMS) {
      request->send(400, "application/json", "{\"error\":\"invalid index\"}");
      return;
    }

    bool repeat[7] = {false, false, false, false, false, false, false};
    if (incoming.containsKey("repeat") && incoming["repeat"].is<JsonArray>()) {
        JsonArray repeatArr = incoming["repeat"].as<JsonArray>();
        for (size_t i = 0; i < 7 && i < repeatArr.size(); ++i) {
            repeat[i] = repeatArr[i].as<bool>();
        }
    }

    // parse existing stored alarms safely
    DynamicJsonDocument doc(4096);
    DeserializationError err2 = deserializeJson(doc, storeAlarm.c_str());
    JsonArray arr;
    if (err2 || !doc.is<JsonArray>()) {
      // start fresh array
      arr = doc.to<JsonArray>();
    } else {
      arr = doc.as<JsonArray>();
    }

    // ensure array has at least idx+1 elements, but cap growth to avoid OOM
    while ((int)arr.size() <= idx && (int)arr.size() < MAX_ALARMS) {
        arr.createNestedObject();
    }

    // Safety: if arr still too small (shouldn't happen because we validated idx),
    // return error
    if ((int)arr.size() <= idx) {
      request->send(500, "application/json", "{\"error\":\"internal error - cannot allocate slot\"}");
      return;
    }

    // assign values to slot
    JsonObject slot = arr[idx].as<JsonObject>();
    slot["received"] = true;
    slot["index"] = idx;
    slot["status"] = status;
    slot["timeH"] = timeH;
    slot["timeM"] = timeM;
    slot["name"] = name;
    slot["repeatS"] = repeatS;

    // replace existing repeat array cleanly
    if (slot.containsKey("repeat")) slot.remove("repeat");
    JsonArray repeatArrOut = slot.createNestedArray("repeat");
    for (int i = 0; i < 7; i++) {
        repeatArrOut.add(repeat[i]);
    }

    bool replaced = false;
    for (size_t i = 0; i < alarms.size(); ++i) {
        if (alarms[i].index == idx) {
            alarms[i].status = status;
            alarms[i].timeH = timeH;
            alarms[i].timeM = timeM;
            alarms[i].name = String(name);
            alarms[i].repeatS = repeatS;
            for (int j = 0; j < 7; ++j) alarms[i].repeat[j] = repeat[j];
            replaced = true;
            break;
        }
    }
    if (!replaced) {
        Alarm newAlarm;
        newAlarm.index = idx;
        newAlarm.status = status;
        newAlarm.timeH = timeH;
        newAlarm.timeM = timeM;
        newAlarm.name = String(name);
        newAlarm.repeatS = repeatS;
        for (int i = 0; i < 7; i++) newAlarm.repeat[i] = repeat[i];
        alarms.push_back(newAlarm);
    }

    Serial.printf("[%u][NEW ALARM idx=%d]\n", (unsigned)alarms.size(), idx);

    String out;
    serializeJson(arr, out);
    storeAlarm = out;
    writeJson("/alarms.json", storeAlarm.c_str());
    writeNVS("alarms", storeAlarm.c_str());
    readJson("/alarms.json");

    request->send(200, "application/json", storeAlarm);
});

    server.on("/data", HTTP_POST, [](AsyncWebServerRequest* request) {}, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      String body;
      body.reserve(len + 1);
      for (size_t i = 0; i < len; ++i) body += (char)data[i];

      DynamicJsonDocument incoming(512);
      DeserializationError err = deserializeJson(incoming, body);
      if (err) {
        request->send(400, "application/json", "{\"error\":\"invalid json\"}");
        return;
      }
      const char* date = incoming["date"] | "";
      const char* duration = incoming["duration"] | "";
      const char* power = incoming["power"] | "";

      DynamicJsonDocument slotDoc(256);
      JsonObject slot = slotDoc.to<JsonObject>();
      slot["date"] = date;
      slot["duration"] = duration;
      slot["power"] = power;

      DynamicJsonDocument doc(4096);
      DeserializationError err2 = deserializeJson(doc, storeLog);
      JsonArray arr;
      if (err2 || !doc.is<JsonArray>()) {
        arr = doc.to<JsonArray>();
      } else {
        arr = doc.as<JsonArray>();
      }

      arr.add(slot);

      if (arr.size() > 100) {
        arr.remove(0);
      }

      Log newLog;
      newLog.duration = duration;
      newLog.date = date;
      newLog.power = power;
      grindLog.push_back(newLog);
      
      String out;
      serializeJson(arr, out);
      storeLog = out;

      writeNVS("logs", storeLog.c_str());
      Serial.println(readNVS("logs"));

      request->send(200, "application/json", storeLog);
    });

    server.on("/erase", HTTP_POST, [](AsyncWebServerRequest* request) {}, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      String body;
      body.reserve(len + 1);
      for (size_t i = 0; i < len; ++i) body += (char)data[i];

      DynamicJsonDocument incoming(512);
      DeserializationError err = deserializeJson(incoming, body);
      if (err) {
        request->send(400, "application/json", "{\"error\":\"invalid json\"}");
        return;
      }
      const char* erase = incoming["erase"] | "";
      const char* type = incoming["type"] | "";

      DynamicJsonDocument slotDoc(256);
      JsonObject slot = slotDoc.to<JsonObject>();
      slot["erase"] = erase;
      slot["type"] = type;
      slot["received"] = true;

      DynamicJsonDocument doc(4096);
      std::string aux;

      DeserializationError err2 = deserializeJson(doc, aux);
      JsonArray arr;
      if (err2 || !doc.is<JsonArray>()) {
        arr = doc.to<JsonArray>();
      } else {
        arr = doc.as<JsonArray>();
      }

      arr.add(slot);

      if (arr.size() > 100) {
        arr.remove(0);
      }

      String out;
      serializeJson(arr, out);

      eraseNVS(type);

      Serial.printf("Erased %s!\r\n", type);

      request->send(200, "application/json", out);
    });

  server.on("/alarms", HTTP_GET, [](AsyncWebServerRequest *request) {
    storeAlarm = readNVS("alarms");
    request->send(200, "application/json", storeAlarm);
  });

  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
    storeLog = readNVS("logs");
    request->send(200, "application/json", storeLog);
  });

  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);

  server.begin();
}