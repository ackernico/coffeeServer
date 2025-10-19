#include "../inc/globals.h"

bool webPower = false;
std::vector<Alarm> alarms;
String storeAlarm = "[]";
long powerSum = 0;
int powerSamples = 0;
irt realTime;