#pragma once
#include <Arduino.h>
#include <vector>

#include "Alarm.h"
#include "irt.h"

struct Alarm;
struct irt;

extern bool webPower;
extern std::vector<Alarm> alarms;
extern String storeAlarm;
extern long powerSum;
extern int powerSamples;
extern irt realTime;
