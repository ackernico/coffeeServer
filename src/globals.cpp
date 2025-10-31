#include "../inc/globals.h"

bool webPower = false;
bool timeSynced = false;
std::vector<Alarm> alarms;
std::vector<Log> grindLog;
String storeAlarm = "[]";
String storeLog = "[]";
long powerSum = 0;
int powerSamples = 0;
irt realTime;
Preferences prefs;