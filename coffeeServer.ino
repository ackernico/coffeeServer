#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESP32Servo.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

#include <string>
#include <vector>
#include <fstream>

#include <time.h>
#include <sntp.h>

#include "inc/webserver.h"
#include "inc/fileManager.h"
#include "inc/timeManager.h"
#include "inc/globals.h"

/*Network credentials*/
const char* ssid = "Nicolas_2.4G";
const char* pass = "nicolas2006";

/*NTP Server credentials*/
const char* ntpServer = "pool.ntp.org";
const long gmtOffsetSec = -10800;
const int  daylightOffsetSec = 0;

/*Variables*/
Servo servo;

std::ofstream file("grinderLog.json");

String grinderState;

bool powerButton;
bool aux = false;
bool started = false;
bool buttonSustain = false;
bool lastButton = false;

static int lowCount = 0;
int offset;

float power;

long sendPower;
long offTimer;
long startTime = 0;
long elapsedTime = 0;

/*Pins declaration*/
const int clockWiseButton = 15;
const int counterClockWiseButton = 2;
const int clockWisePin = 13;
const int counterClockWisePin = 12;
const int servoPin = 18;
const int measurePin = 35;
const int ntpPin = 25;

void turnON(bool webButton, int thickness = 11)
{
  if(!webButton) turnOFF();
  else
  {
    digitalWrite(clockWisePin, HIGH);
  }
}

void turnOFF()
{
  offTimer = millis();
  digitalWrite(clockWisePin, LOW);
  servo.write(45);
}

float analogFilter(int pin, int samples, bool calibrate = false)
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
  pinMode(ntpPin, OUTPUT);

  digitalWrite(ntpPin, LOW);

  sntp_set_time_sync_notification_cb(timeavailable);
  configTime(gmtOffsetSec, 0, ntpServer);

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

  Serial.print("Connecting to NTP server");
  while(storeTime(false) != 0) Serial.print(".");
  Serial.println("Connected!");

  offset = analogFilter(measurePin, 1200, true);

  listDir();

  configServer();
}

void loop() 
{
  powerButton = digitalRead(clockWiseButton);
  storeTime(false);

  if(powerButton && !lastButton)
  {
    buttonSustain = !buttonSustain;
    ws.textAll("{\"state\":\"true\"}");
  } 
  lastButton = powerButton;

  turnON(webPower || buttonSustain);

  if(webPower || buttonSustain)
  {
    if(!started)
    {
      startTime = millis();
      started = true;
    }
    else
    {
      elapsedTime = millis() - startTime;
    } 

    if(elapsedTime < 10000) servo.write(25);
    else if(elapsedTime > 10000 && elapsedTime < 30000) servo.write(15);
    else if (elapsedTime > 50000) servo.write(0);

    if(millis() - sendPower >= 400)
    {
      power = analogFilter(measurePin, 2500);
      String socketMsg = "{\"power\":\"" + String(power) + "\"}";
      ws.textAll(socketMsg);
      sendPower = millis();
      
      powerSum += power;
      powerSamples++;
    }
    
    if((power) <= 1.68)
    {
      if(!aux)
      {
        aux = true;
        offTimer = millis();
      }
      else
      {
        lowCount++;
      }

      if((millis() - offTimer >= 4500) && lowCount >= 10)
      {
        turnOFF();
        if(elapsedTime >= 10000) ws.textAll("{\"register\":\"true\"}");
        ws.textAll("{\"state\":\"true\"}");
        Serial.println("Button turned off!");
        webPower = false;
        buttonSustain = false;
        aux = false;
        powerSamples = 0;
        powerSum = 0;
      }
    }
    else
    {
      aux = false;
      lowCount = 0;
    }
  }
  else
  {
    if(started)
    {
      started = false;
      elapsedTime = 0;
    }
  }

  for(int i=0 ; i<alarms.size() ; i++)
  {
    if(!alarms[i].triggered && alarms[i].timeH == realTime.hour && alarms[i].timeM == realTime.minute && alarms[i].status)
    {
      turnON(true, alarms[i].thickness);
      ws.textAll("{\"state\":\"true\"}");
      alarms[i].triggered = true;
      Serial.println(alarms[i].name);
    }

    if(alarms[i].timeM != realTime.minute) alarms[i].triggered = false;
  }
}
