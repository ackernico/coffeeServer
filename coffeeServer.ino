#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

/*Network credentials*/
const char* ssid = "Nicolas_2.4G";
const char* pass = "nicolas2006";

/*Pins declaration*/
const int clockWisePin = 33;

/*Auxiliar variables*/
String grinderState;
String storeAlarm = "[]";

AsyncWebServer server(80);

void listDir(fs::FS &fs = LittleFS, const char * dirname = "/", uint8_t levels = 1)
{
    File root = fs.open(dirname);
    if(!root)
    {
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory())
    {
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file)
    {
        if(file.isDirectory())
        {
            Serial.print("  DIR : ");
            Serial.printf("    %s\n", file.name());
            if(levels)
            {
                listDir(fs, file.path(), levels -1);
            }
        } 
        else 
        {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void writeJson(const char* path, const char* message, fs::FS &fs = LittleFS)
{
  Serial.printf("Writing file on %s\r\n", path);

  File alarmJSON;
  alarmJSON = fs.open(path, FILE_WRITE);

  if(!alarmJSON)
  {
    Serial.println("Error while opening file");
    return;
  }

  if(alarmJSON.print(message))
  {
    Serial.println("Alarms stored!");
  }
  else Serial.println("Error while writing file");
  
  alarmJSON.close();
}

void readJson(const char* path, fs::FS &fs = LittleFS)
{
  Serial.printf("Reading file on %s\r\n", path);

  File alarmJSON;
  alarmJSON = fs.open(path);

  size_t fileSize = alarmJSON.size();
  char* outJSON = new char[fileSize + 1];
  
  if(!alarmJSON)
  {
    Serial.println("Error while opening file");
    return;
  }

  size_t i = 0;
  while(alarmJSON.available())
  {
    *(outJSON + i) = (char)alarmJSON.read();
    i++;
  }
  outJSON[i] = '\0';

  storeAlarm = outJSON;
  Serial.println(storeAlarm);

  Serial.println("Alarms read!");
  delete[] outJSON;
  alarmJSON.close();
}

void setup() 
{
  Serial.begin(115200);

  pinMode(clockWisePin, OUTPUT);

  if(!LittleFS.begin(true))
  {
    Serial.println("An error has occurred while mounting LittleFS");
    return;
  }

  WiFi.begin(ssid, pass);
  while(WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected!");
  Serial.println(WiFi.localIP());

  listDir();

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

  // HTML files route
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/html/index.html", String(), false);
  });
  
  // Partials files route
  server.serveStatic("/html", LittleFS, "/html/");
  server.serveStatic("/css", LittleFS, "/css/");

  // JS file route
   server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/script.js", "text/javascript");
  });
  // Font file route
  server.on("/Inter.ttf", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/Inter.ttf", "text/ttf");
  });
  
  server.on("/on", HTTP_POST, [](AsyncWebServerRequest *request){},
  NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) 
    {
      Serial.println("Ring Ring Ring! POST method is calling!");
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

      Serial.printf("Status: %s\n", status);
      Serial.printf("Thickness: %s\n", thickness);

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
      Serial.println("Received alarms POST: " + body);

      DynamicJsonDocument incoming(512);
      if (deserializeJson(incoming, body)) {
        request->send(400, "application/json", "{\"error\":\"invalid json\"}");
        return;
      }

      int idx = incoming["index"] | -1;
      bool status = incoming["status"] | false;
      const char* time = incoming["time"] | "";
      const char* name = incoming["name"] | "";
      
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
      slot["time"] = time;
      slot["name"] = name;

      String out;
      serializeJson(arr, out);
      storeAlarm = out;
      writeJson("/alarms.json", storeAlarm.c_str());
      readJson("/alarms.json");

      request->send(200, "application/json", storeAlarm);
    });

  server.on("/alarms", HTTP_GET, [](AsyncWebServerRequest *request) {
    readJson("/alarms.json");
    request->send(200, "application/json", storeAlarm);
  });
  
  server.begin();
}

void loop() 
{
  // put your main code here, to run repeatedly:
}
