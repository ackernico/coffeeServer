#include <WiFi.h>
#include <ESP32Servo.h>
#include "time.h"
#include "sntp.h"

#define SSID "Nicolas"
#define PASS "04122004"

Servo servo;

const char* ntpServer = "pool.ntp.org";
const long gmtOffsetSec = -10800;
const int  daylightOffsetSec = 0;

int presetMinute = 15;
int presetHour = 11;

int diaSemana = 0;
int hora = 0;
int minuto = 0;

int servoPosition = 0;

const int servoPin = 18;
const int button1Pin = 15;

void testaServo()
{
  for(int i=0 ; i < 180 ; i++)
  {
    servo.write(i);
    Serial.print("Servo na posicao ");
    Serial.println(i);
    delay(20);
  }

  for(int i=180 ; i > 0 ; i--)
  {
    servo.write(i);
    Serial.print("Servo na posicao ");
    Serial.println(i);
    delay(20);
  }
}

int storeTime(bool printTime, int* wDay, int* hour, int* min)
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo))
  {
    Serial.println("Obtendo tempo...");
    return -1;
  } 
  if(printTime) Serial.println(&timeinfo, "%B %d %Y %H:%M:%S");
  
  diaSemana = timeinfo.tm_wday;
  hora      = timeinfo.tm_hour;
  minuto    = timeinfo.tm_min;

  return 0;
}

void timeavailable(struct timeval *t)
{
  Serial.println("Got time adjustment from NTP!");
  storeTime(0, 0, 0, 0);
}

void setup() 
{
  Serial.begin(115200);
  Serial.printf("Conectando-se a '%s'", SSID);
  WiFi.begin(SSID, PASS);

  while(WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Conectado!");

  ESP32PWM::allocateTimer(0);
  servo.setPeriodHertz(50);
  servo.attach(servoPin, 1000, 2000);
  servo.write(0);

  pinMode(button1Pin, INPUT_PULLDOWN);

  sntp_set_time_sync_notification_cb(timeavailable);
  configTime(gmtOffsetSec, 0, ntpServer);
}

void loop() 
{
  delay(5);

  if(storeTime(0, &diaSemana, &hora, &minuto) == 0)
  {
    Serial.print("Dia semana: ");
    Serial.println(diaSemana);
    Serial.print("Hora: ");
    Serial.println(hora);
    Serial.print("Minuto: ");
    Serial.println(minuto);
  }

  if((hora == presetHour) && (minuto == presetMinute))
  {
    servo.write(180);
    Serial.println("Servo aberto!");
  }
  else
  {
    servo.write(0);
    Serial.println("Servo fechado!");
  }
}
