#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Buzzer.h>

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
const int daylightOffsetSec = 0;

/*Variables*/
Servo servo;
LiquidCrystal_I2C lcd(0x27, 20, 4);
Buzzer buzzer(33);

String grinderState;

enum DisplayMode { NORMAL,
                   ADD_ALARM };
DisplayMode currentMode = NORMAL;

bool powerButton;
bool aux = false;
bool started = false;
bool buttonSustain = false;
bool lastButton = false;
bool isConnected = false;
bool NTPmsg = false;
bool Wifimsg = false;
bool ntpShownOnce = false;
bool lcdRealTime = false;
bool buttonPressed = false;
bool alarmMode = false;
bool editingHour = true;
bool isProcessing = false;
bool startedProcessing = false;

static int lowCount = 0;
static int lastCheckedMinute = -1;
int offset;
int lcdTime = -1;
int lastCLKState;
int counter = 0;
int editHour = 0;
int editMinute = 0;
int processingSeconds = 0;
int processingMinutes = 0;
int proccesingPrint = 0;

float power;

long sendPower;
long offTimer;
long processTimer = 0;
long startTime = 0;
long elapsedTime = 0;
long connectTime = 0;
long clearWifi = 0;
long clearNTP = 0;
unsigned long lastTurnTime = 0;
unsigned long lastButtonTime = 0;

/*Pins declaration*/
const int clockWiseButton = 15;
const int alarmButton = 2;
const int clockWisePin = 13;
const int counterClockWisePin = 12;
const int servoPin = 18;
const int measurePin = 35;
const int ntpPin = 25;
const int wifiPin = 32;
const int buzzPin = 33;
const int clkPin = 23;
const int dtPin = 19;
const int swPin = 5;

byte loadingBar[5][8] = {
  { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10 },
  { 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18 },
  { 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C },
  { 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E },
  { 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F }
};

byte coffeeMug[] = {
  0x0A, 0x14, 0x00, 0x1E, 0x1D, 0x1D, 0x1E, 0x1C
};

byte clockChar[] = {
  0x0E, 0x11, 0x15, 0x15, 0x17, 0x11, 0x11, 0x0E
};

byte wifiChar[] = {
  0x0E, 0x11, 0x04, 0x0A, 0x11, 0x04, 0x0E, 0x04
};

void soundON()
{
 buzzer.sound(NOTE_G4, 75);
 buzzer.sound(NOTE_C5, 75);
 buzzer.sound(NOTE_E4, 90);
}

void successSound()
{
  buzzer.sound(NOTE_C5, 150);
  buzzer.sound(NOTE_F5, 90);
}

void turnON(bool webButton) {
  if (!webButton) turnOFF();
  else {
    digitalWrite(clockWisePin, HIGH);
    isProcessing = true;
  }
}

void turnOFF() {
  offTimer = millis();
  digitalWrite(clockWisePin, LOW);
  servo.write(45);
  isProcessing = false;
}

float analogFilter(int pin, int samples, bool calibrate = false) {
  long sum = 0;
  for (int i = 0; i < samples; i++)
    sum += analogRead(pin);

  float avg = (float)sum / samples;

  if (calibrate) {
    Serial.printf("Offset calibrated to %.2f\r\n", avg);
    return avg;
  }

  float diff = (avg * 3.3) / 4095.0 - (offset * 3.3) / 4095.0;
  float current = diff / 0.066;

  return current;
}

int initialMessage(bool prinTime) {
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("CoffeeServer");
  lcd.setCursor(14, 0);
  lcd.write(byte(6));
  if (prinTime) {
    lcd.setCursor(5, 1);
    if (realTime.hour <= 9) {
      lcd.print("0");
      lcd.setCursor(6, 1);
    }
    lcd.print(realTime.hour);
    lcd.setCursor(7, 1);
    lcd.print(":");
    lcd.setCursor(8, 1);
    if (realTime.minute <= 9) {
      lcd.print("0");
      lcd.setCursor(9, 1);
    }
    lcd.print(realTime.minute);

    aux = realTime.minute;
    return aux;
  }

  return -1;
}

void updateEncoder() {
  static int lastCLK = digitalRead(clkPin);
  static int lastDT = digitalRead(dtPin);

  int currentCLK = digitalRead(clkPin);
  int currentDT = digitalRead(dtPin);

  if (currentCLK != lastCLK && currentCLK == LOW) {
    if (currentDT != currentCLK) {
      counter++;
      Serial.println("CW");
    } else {
      counter--;
      Serial.println("CCW");
    }
    Serial.printf("Encoder: %d\n", counter);
  }

  lastCLK = currentCLK;
  lastDT = currentDT;
}

void setup() {
  Serial.begin(115200);
  buzzer.begin(100);

  ESP32PWM::allocateTimer(0);
  servo.setPeriodHertz(50);
  servo.attach(servoPin, 1000, 2000);
  servo.write(45);

  lcd.init();
  lcd.backlight();
  for (int i = 0; i < 5; i++) {
    lcd.createChar(i, loadingBar[i]);
  }
  lcd.createChar(6, coffeeMug);
  lcd.createChar(7, clockChar);
  lcd.createChar(8, wifiChar);

  initialMessage(false);

  pinMode(clockWiseButton, INPUT);
  pinMode(alarmButton, INPUT);
  pinMode(clkPin, INPUT_PULLUP);
  pinMode(dtPin, INPUT_PULLUP);
  pinMode(swPin, INPUT_PULLUP);

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

  if (!LittleFS.begin(true)) {
    Serial.println("An error has occurred while mounting LittleFS");
    return;
  }

  WiFi.begin(ssid, pass);

  offset = analogFilter(measurePin, 1200, true);

  lastCLKState = digitalRead(clkPin);

  listDir();
  configServer();
  loadAlarms();
  loadLogs();

  soundON();
}

void loop() {
  int currentCLKState = digitalRead(clkPin);
  int currentDTState = digitalRead(dtPin);

  if (digitalRead(alarmButton) && currentMode == NORMAL) 
  {
    currentMode = ADD_ALARM;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Add new alarm:");
    buzzer.sound(NOTE_F7, 80);
    lcd.blink_on();
    counter = 0;
    Serial.println("Modo: Adicao de Alarme");
    delay(200);
  }

  if (currentMode == ADD_ALARM) 
  {
    updateEncoder();

    if (editingHour) {
      editHour = constrain(counter, 0, 23);
    } else {
      editMinute = constrain(counter, 0, 59);
    }

    lcd.setCursor(5, 1);
    if (editHour < 10) lcd.print("0");
    lcd.print(editHour);
    lcd.print(":");
    if (editMinute < 10) lcd.print("0");
    lcd.print(editMinute);

    if (editingHour)
      lcd.setCursor(5, 1);
    else
      lcd.setCursor(7, 1);

    if (!digitalRead(swPin) && millis() - lastButtonTime > 300) 
    {
      lastButtonTime = millis();
      editingHour = !editingHour;

      if (!editingHour) counter = editMinute;
      else counter = editHour;
    }

    if (digitalRead(alarmButton) && millis() - lastButtonTime > 1000) 
    {
      currentMode = NORMAL;
      buzzer.sound(NOTE_B7, 80);
      lcd.blink_off();
      lcd.clear();
      initialMessage(true);
      delay(200);
      Serial.println("Saiu do modo de adicao");
    }
    return;
  }

  if (timeSynced && !isProcessing) 
  {
    if (storeTime(false) == 0) digitalWrite(ntpPin, HIGH);

    if (!ntpShownOnce) 
    {
      lcd.setCursor(0, 1);
      lcd.write(byte(7));
      lcd.setCursor(1, 1);
      lcd.print("NTP connected!");
      successSound();
      clearNTP = millis();
      NTPmsg = true;
      ntpShownOnce = true;
    }
  }

  if (timeSynced && !isProcessing) 
  {
    storeTime(false);
    if (realTime.minute != lcdTime) 
    {
      lcdTime = realTime.minute;
      initialMessage(true);
    }
  }

  if (timeSynced && !isProcessing) {
    storeTime(false);
    if (realTime.minute != lastCheckedMinute) {
      lastCheckedMinute = realTime.minute;
      Serial.printf("[ALARM] Checking alarms for %02d:%02d (count=%u)\n",
                    realTime.hour, realTime.minute, (unsigned)alarms.size());

      for (size_t i = 0; i < alarms.size(); ++i) {
        Alarm& a = alarms[i];
        Serial.printf("[ALARM] idx=%u name=\"%s\" status=%d triggered=%d alarm=%02d:%02d\n",
                      (unsigned)i, a.name.c_str(), (int)a.status, (int)a.triggered, a.timeH, a.timeM);

        if (!a.status) continue;
        if (a.timeH == realTime.hour && a.timeM == realTime.minute && !a.triggered) {
          webPower = true;
          ws.textAll("{\"state\":\"true\"}");
          a.triggered = true;
          Serial.printf("[ALARM] TRIGGER idx=%u name=\"%s\"\n", (unsigned)i, a.name.c_str());
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.write(byte(7));
          lcd.setCursor(1, 0);
          lcd.print("Alarm ringing!");
          lcd.setCursor(0, 1);
          lcd.print(a.name.c_str());
        }
      }
    }
  }

  for (size_t i = 0; i < alarms.size(); ++i) {
    if (alarms[i].timeM != realTime.minute) alarms[i].triggered = false;
  }

  if (NTPmsg && millis() - clearNTP >= 2000) {
    lcd.clear();
    lcdTime = initialMessage(true);
    NTPmsg = false;
    lcdRealTime = true;
  }

  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - connectTime >= 500) {
      Serial.print(".");
      connectTime = millis();
    }
  } else {
    if (!isConnected) {
      Serial.println("Connected to WiFi!");
      digitalWrite(wifiPin, HIGH);
      Serial.println(WiFi.localIP());
      lcd.setCursor(0, 0);
      lcd.write(byte(8));
      lcd.setCursor(1, 0);
      lcd.print("WiFi connected!");
      lcd.setCursor(2, 1);
      lcd.print(WiFi.localIP());
      successSound();
      clearWifi = millis();
      isConnected = true;
      Wifimsg = true;
    }
  }

  if (Wifimsg && millis() - clearWifi >= 3500) {
    lcd.clear();
    initialMessage(false);
    Wifimsg = false;
  }

  powerButton = digitalRead(clockWiseButton);
  if (storeTime(false) == 0) digitalWrite(ntpPin, HIGH);

  if (powerButton && !lastButton) {
    buttonSustain = !buttonSustain;
    ws.textAll("{\"state\":\"true\"}");
  }
  lastButton = powerButton;

  turnON(webPower || buttonSustain);

  if (webPower || buttonSustain) {
    if (!started) {
      startTime = millis();
      started = true;
    } else {
      elapsedTime = millis() - startTime;
    }

    if (elapsedTime < 1000) servo.write(19);
    else if (elapsedTime > 1000 && elapsedTime < 6000) servo.write(25);
    else if (elapsedTime > 6000 && elapsedTime < 12000) servo.write(20);
    else if (elapsedTime > 12000 && elapsedTime < 20000) servo.write(15);
    else if (elapsedTime > 20000 && elapsedTime < 30000) servo.write(5);
    else if (elapsedTime > 40000) servo.write(0);

    if (millis() - sendPower >= 400) {
      power = analogFilter(measurePin, 2500);
      String socketMsg = "{\"power\":\"" + String(power) + "\"}";
      ws.textAll(socketMsg);
      sendPower = millis();

      powerSum += (power * 100);
      powerSamples++;
    }

    if ((power) <= 1.68) {
      if (!aux) {
        aux = true;
        offTimer = millis();
      } else {
        lowCount++;
      }

      if ((millis() - offTimer >= 4500) && lowCount >= 10) {
        turnOFF();
        String auxMsg = "{\"register\":\"true\",\"avgPower\":\"" + String(((float)powerSum / (float)powerSamples) / 100) + "\"}";
        if (elapsedTime >= 10000) ws.textAll(auxMsg);
        ws.textAll("{\"state\":\"true\"}");
        Serial.println("Button turned off!");
        webPower = false;
        buttonSustain = false;
        aux = false;
        powerSamples = 0;
        powerSum = 0;
        lcd.clear();
        if(timeSynced) lcdTime = initialMessage(true);
        else initialMessage(false);
        startedProcessing = false;
      }
    } else {
      aux = false;
      lowCount = 0;
    }
  } else {
    if (started) {
      started = false;
      elapsedTime = 0;
    }
  }

  if (isProcessing) 
  {
    if (!startedProcessing) 
    {
      processTimer = millis();    
      startedProcessing = true;

      processingSeconds = 0;
      processingMinutes = 0;

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Grinding");
      lcd.setCursor(11, 0);
      lcd.print("00:00");
    }

    if (millis() - processTimer >= 1000) 
    {
      processTimer = millis(); 
      processingSeconds++;

      if (processingSeconds >= 60) 
      {
        processingMinutes++;
        processingSeconds = 0;
      }

      lcd.setCursor(11, 0);
      if (processingMinutes < 10) lcd.print("0");
      lcd.print(processingMinutes);
      lcd.print(":");
      if (processingSeconds < 10) lcd.print("0");
      lcd.print(processingSeconds);
    }
  }
}
