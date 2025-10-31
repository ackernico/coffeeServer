#pragma once
#include <Arduino.h>
#include <vector>
#include <Preferences.h>

#include "Alarm.h"
#include "Log.h"
#include "irt.h"

struct Alarm;
struct Log;
struct irt;

extern bool webPower;
extern bool timeSynced;
extern std::vector<Alarm> alarms;
extern std::vector<Log> grindLog;
extern String storeAlarm;
extern String storeLog;
extern long powerSum;
extern int powerSamples;
extern irt realTime;
extern Preferences prefs;
