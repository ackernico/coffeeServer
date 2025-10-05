#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <ESP32Servo.h>
#include <string>
#include <iostream>

/*Network credentials*/
const char* ssid = "Nicolas_2.4G";
const char* pass = "nicolas2006";

/*Auxiliar variables*/
String grinderState;
String storeAlarm = "[]";
Servo servo;
bool powerButton;
int offset;

/*Pins declaration*/
const int clockWiseButton = 15;
const int counterClockWiseButton = 2;
const int clockWisePin = 13;
const int counterClockWisePin = 12;
const int servoPin = 18;
const int measurePin = 35;

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

void turnON(int thickness = 11)
{
  servo.write(25);
  digitalWrite(clockWisePin, HIGH);
}

int analogFilter(int pin, int samples, bool calibrate = false)
{
  long sum = 0;
  int aux;
  float finalMeasure;

  for(int i=0 ; i<=samples ; i++)
  {
    sum += analogRead(pin);
  }

  aux = sum/samples;
  if(calibrate)
  {
    Serial.printf("Offset calibrated to %i\r\n", aux);
    return aux;
  } 
  else if(!calibrate)
  {
    finalMeasure = ((((float)aux - offset) * 3.3)/(float)4095) * 10;
    Serial.printf("Current: %.2f A\r\n", finalMeasure);
    return finalMeasure;
  }
}

void setup() 
{
  Serial.begin(115200);

  ESP32PWM::allocateTimer(0);
  servo.setPeriodHertz(50);
  servo.attach(servoPin, 1000, 2000);
  servo.write(45);

  pinMode(clockWiseButton, INPUT);
  pinMode(counterClockWiseButton, INPUT);
  pinMode(clockWisePin, OUTPUT);
  pinMode(counterClockWisePin, OUTPUT);

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
  offset = analogFilter(measurePin, 1200, true);

  listDir();

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

  // HTML files route
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/html/index.html", String(), false);
  });
  
  // Partials files route
  server.serveStatic("/html", LittleFS, "/html/");
  server.serveStatic("/css", LittleFS, "/css/");
  server.serveStatic("/assets", LittleFS, "/assets/");

  // JS file route
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
      const char* timeH = incoming["timeH"] | "";
      const char* timeM = incoming["timeM"] | "";
      const char* name = incoming["name"] | "";
      bool repeat[7] = {false, false, false, false, false, false, false};
      if (incoming.containsKey("repeat") && incoming["repeat"].is<JsonArray>()) {
          JsonArray repeatArr = incoming["repeat"].as<JsonArray>();
          for (size_t i = 0; i < 7 && i < repeatArr.size(); ++i) {
              repeat[i] = repeatArr[i].as<bool>();
          }
          // Now repeat[] contains your 7 booleans
      }
      const char* repeatS = incoming["repeatS"] | "";
      
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
      slot["repeat"] = repeat;
      slot["repeatS"] = repeatS;

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
  powerButton = digitalRead(clockWiseButton);

  if(powerButton) turnON();
}
