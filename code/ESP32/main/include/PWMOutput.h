#pragma once
#include <Arduino.h>
#include <sstream>
#include <string>
#include "defines.h"

namespace CLASSICDIY {
class PWMOutput {
 public:
   PWMOutput(int pin);
   ~PWMOutput();

 public:
   void SetDutyCycle(uint8_t dutyCycle);
   uint8_t GetDutyCycle();

 private:
   int _pin;
   uint8_t _dutyCycle;
};
} // namespace CLASSICDIY
