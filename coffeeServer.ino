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

 /*Global variables to store the webpage data*/

/*Creating AsyncWebServer object on port 80*/
AsyncWebServer server(80);

void setup() {
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

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", String(), false);
  });
  
  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/style.css", "text/css");
  });

   server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/script.js", "text/js");
  });

  server.on("/Inter.ttf", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/Inter.ttf", "text/ttf");
  });
  
  // Route to set GPIO to LOW
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
  
  server.begin();
}

void loop() 
{
  // put your main code here, to run repeatedly:
}
