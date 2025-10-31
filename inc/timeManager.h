#pragma once
#include <time.h>
#include <sntp.h>

int storeTime(bool printTime);
void timeAvailable(struct timeval *t);
