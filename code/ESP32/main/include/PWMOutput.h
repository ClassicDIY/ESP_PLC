#pragma once
#include <Arduino.h>
#include <sstream>
#include <string>
#include "IOT_Defines.h"

class PWMOutput {
 public:
   PWMOutput(int pin);
   ~PWMOutput();

 public:
   void SetDutyCycle(uint8_t dutyCycle);

 private:
   int _pin;
   uint8_t _dutyCycle;
};

