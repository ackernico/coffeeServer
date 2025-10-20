#pragma once
#include <LittleFS.h>

#include "globals.h"

void listDir(fs::FS &fs = LittleFS, const char * dirname = "/", uint8_t levels = 1);
void writeJson(const char* path, const char* message, fs::FS &fs = LittleFS);
void readJson(const char* path, fs::FS &fs = LittleFS);
void writeNVS(const char* key, const String& json);
String readNVS(const char* key);
void eraseNVS(const char* key);

