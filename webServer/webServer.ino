#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "Nicolas_2.4G";
const char* pass = "nicolas2006";

WebServer server(80);

void webpage()
{
  server.send(200, "text/plain", "Test Server");
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, pass);
  Serial.printf("Connecting to '%s'", ssid);

  while(WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected!"); 
  
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", webpage);

  server.begin();
  Serial.println("Server initialized!");
}

void loop() {
  server.handleClient();
}


