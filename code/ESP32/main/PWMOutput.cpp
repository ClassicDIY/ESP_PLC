#include <Arduino.h>
#include "Log.h"
#include "Defines.h"
#include "PWMOutput.h"

namespace CLASSICDIY {
PWMOutput::PWMOutput(int pin) {
   _pin = pin;
   pinMode(_pin, OUTPUT);
   _dutyCycle = 0;
   analogWrite(_pin, _dutyCycle);
}

PWMOutput::~PWMOutput() {}

void PWMOutput::SetDutyCycle(uint8_t dutyCycle) {
   _dutyCycle = dutyCycle;
   analogWrite(_pin, dutyCycle);
   return;
}

} // namespace CLASSICDIY
