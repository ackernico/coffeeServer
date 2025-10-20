#include "../inc/fileManager.h"
#include "../inc/globals.h"


#include <LittleFS.h>
#include <Arduino.h>
#include <Preferences.h>

void listDir(fs::FS &fs, const char * dirname, uint8_t levels)
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

void writeJson(const char* path, const char* message, fs::FS &fs)
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
    writeNVS(path, message);
  }
  else Serial.println("Error while writing file");
  
  alarmJSON.close();
}

void readJson(const char* path, fs::FS &fs)
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

void writeNVS(const char* key, const String& json)
{
  prefs.begin(key, false);
  prefs.putString("data", json);
  prefs.end();
  Serial.println("String stored on non volatile!");
}

String readNVS(const char* key)
{
  prefs.begin(key, true);
  String data = prefs.getString("data", "{}");

  prefs.end();
  Serial.println("String read on non volatile!");

  return data;
}

void eraseNVS(const char* key)
{
  prefs.begin(key, false);
  prefs.clear();
  prefs.end();
  Serial.println("Cleared!");
}