#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

#include <string>
#include <vector>

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
LiquidCrystal_I2C lcd(0x27, 20, 4);

String grinderState;

bool powerButton;
bool aux = false;
bool started = false;
bool buttonSustain = false;
bool lastButton = false;
bool isConnected = false;
bool NTPmsg = false;
bool Wifimsg = false;
bool ntpShownOnce = false;

static int lowCount = 0;
int offset;

float power;

long sendPower;
long offTimer;
long startTime = 0;
long elapsedTime = 0;
long connectTime = 0;
long clearWifi = 0;
long clearNTP = 0;

/*Pins declaration*/
const int clockWiseButton = 15;
const int counterClockWiseButton = 2;
const int clockWisePin = 13;
const int counterClockWisePin = 12;
const int servoPin = 18;
const int measurePin = 35;
const int ntpPin = 25;
const int wifiPin = 32;
const int buzzPin = 33;

byte loadingBar[5][8] = {
  {0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10},
  {0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18},
  {0x1C,0x1C,0x1C,0x1C,0x1C,0x1C,0x1C,0x1C},
  {0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E},
  {0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F}
};

byte coffeeMug[] = 
{
  0x0A,0x14,0x00,0x1E,0x1D,0x1D,0x1E,0x1C
};

byte clockChar[] = 
{
  0x0E,0x11,0x15,0x15,0x17,0x11,0x11,0x0E
};

byte wifiChar[] = 
{
  0x0E,0x11,0x04,0x0A,0x11,0x04,0x0E,0x04
};

void turnON(bool webButton)
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
  for(int i = 0; i < samples; i++)
    sum += analogRead(pin);

  float avg = (float)sum / samples;

  if(calibrate)
  {
    Serial.printf("Offset calibrated to %.2f\r\n", avg);
    return avg;
  }

  float diff = (avg * 3.3) / 4095.0 - (offset * 3.3) / 4095.0;                        
  float current = diff / 0.066;     

  return current;
}

void initialMessage()
{
  lcd.clear();
  lcd.setCursor(1,0);
  lcd.print("CoffeeServer");
  lcd.setCursor(14,0);
  lcd.write(byte(6));
}

void setup() 
{
  Serial.begin(115200);

  ESP32PWM::allocateTimer(0);
  servo.setPeriodHertz(50);
  servo.attach(servoPin, 1000, 2000);
  servo.write(45);

  lcd.init();
  lcd.backlight();
  for(int i=0 ; i<5 ; i++)
  {
    lcd.createChar(i, loadingBar[i]);
  }
  lcd.createChar(6, coffeeMug);
  lcd.createChar(7, clockChar);
  lcd.createChar(8, wifiChar);

  initialMessage();

  pinMode(clockWiseButton, INPUT);
  pinMode(counterClockWiseButton, INPUT);

  pinMode(clockWisePin, OUTPUT);
  pinMode(counterClockWisePin, OUTPUT);
  pinMode(wifiPin, OUTPUT);
  pinMode(ntpPin, OUTPUT);
  pinMode(buzzPin, OUTPUT);

  digitalWrite(wifiPin, LOW);
  digitalWrite(ntpPin, LOW);
  digitalWrite(buzzPin, LOW);

  sntp_set_time_sync_notification_cb(timeAvailable);
  configTime(gmtOffsetSec, 0, ntpServer);

  if(!LittleFS.begin(true))
  {
    Serial.println("An error has occurred while mounting LittleFS");
    return;
  }

  WiFi.begin(ssid, pass);

  offset = analogFilter(measurePin, 1200, true);

  listDir();
  configServer();
  loadAlarms();
  loadLogs();
}

void loop() 
{
  if (timeSynced && !NTPmsg && !ntpShownOnce)
  {
    if (storeTime(false) == 0)
    {
      lcd.setCursor(0, 1);
      lcd.write(byte(7));
      lcd.setCursor(1, 1);
      lcd.print("NTP connected!");
      digitalWrite(ntpPin, HIGH);
      clearNTP = millis();
      NTPmsg = true;
      ntpShownOnce = true;
    }
  }

  if (NTPmsg && millis() - clearNTP >= 2000)
  {
    lcd.clear();
    initialMessage();
    NTPmsg = false;
  }

  if(WiFi.status() != WL_CONNECTED)
  {
    if(millis() - connectTime >= 500)
    {
      Serial.print(".");
      connectTime = millis();
    }
  }
  else
  {
    if(!isConnected)
    {
      Serial.println("Connected to WiFi!");
      digitalWrite(wifiPin, HIGH);
      Serial.println(WiFi.localIP());
      lcd.setCursor(0, 1);
      lcd.write(byte(8));
      lcd.setCursor(1, 1);
      lcd.print("WiFi connected!");
      clearWifi = millis();
      isConnected = true;
      Wifimsg = true;
    }
  }

  if(Wifimsg && millis() - clearWifi >= 2000)
  {
    lcd.clear();
    initialMessage();
    Wifimsg = false;
  }
  
  powerButton = digitalRead(clockWiseButton);
  if(storeTime(false) == 0) digitalWrite(ntpPin, HIGH);

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

    if(elapsedTime < 6000) servo.write(30);
    else if(elapsedTime > 6000 && elapsedTime < 12000) servo.write(25);
    else if(elapsedTime > 12000 && elapsedTime < 20000) servo.write(20);
    else if(elapsedTime > 20000 && elapsedTime < 30000) servo.write(10);
    else if (elapsedTime > 40000) servo.write(0);

    if(millis() - sendPower >= 400)
    {
      power = analogFilter(measurePin, 2500);
      String socketMsg = "{\"power\":\"" + String(power) + "\"}";
      ws.textAll(socketMsg);
      sendPower = millis();
      
      powerSum += (power*100);
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
        String auxMsg = "{\"register\":\"true\",\"avgPower\":\"" + String(((float)powerSum/(float)powerSamples)/100) + "\"}";
        if(elapsedTime >= 5) ws.textAll(auxMsg);
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
      webPower = true;
      ws.textAll("{\"state\":\"true\"}");
      alarms[i].triggered = true;
      Serial.println(alarms[i].name);
    }

    if(alarms[i].timeM != realTime.minute) alarms[i].triggered = false;
  }
}
