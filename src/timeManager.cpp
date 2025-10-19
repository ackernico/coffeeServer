#include "../inc/timeManager.h"
#include "../inc/globals.h"
#include <time.h>
#include <sntp.h>
#include <Arduino.h>

int storeTime(bool printTime)
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo))
  {
    return -1;
  } 
  if(printTime) Serial.println(&timeinfo, "%B %d %Y %H:%M:%S");
  
  realTime.weekDay   = timeinfo.tm_wday;
  realTime.hour      = timeinfo.tm_hour;
  realTime.minute    = timeinfo.tm_min;

  return 0;
}

void timeavailable(struct timeval *t)
{
  Serial.println("Got time adjustment from NTP!");
  storeTime(false);
}