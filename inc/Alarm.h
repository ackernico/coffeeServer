#pragma once
#include <Arduino.h>

struct Alarm {
  int index;
  bool status;
  int timeH;
  int timeM;
  String name;
  int thickness;
  String repeatS;
  bool repeat[7];
  bool triggered;
};
