#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

/*Network credentials*/
const char* ssid = "Nicolas_2.4G";
const char* pass = "nicolas2006";

/*Pins declaration*/
const int clockWisePin = 33;

/*Auxiliar variables*/
String grinderState;

/*Creating AsyncWebServer object on port 80*/
AsyncWebServer server(80);

String processor(const String& var)
{
  Serial.println(var);
  if(var == "STATE")
  {
    if(digitalRead(clockWisePin))
    {
      grinderState = "ON";
    }
    else
    {
      grinderState = "OFF";
    }
    Serial.println(grinderState);
    return grinderState;
  }
  return String();
}

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

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", String(), false, processor);
  });
  
  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/style.css", "text/css");
  });

  server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(clockWisePin, HIGH);    
    request->send(LittleFS, "/index.html", String(), false, processor);
  });
  
  // Route to set GPIO to LOW
  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(clockWisePin, LOW);    
    request->send(LittleFS, "/index.html", String(), false, processor);
  });

  server.begin();
}

void loop() {
  // put your main code here, to run repeatedly:

}
