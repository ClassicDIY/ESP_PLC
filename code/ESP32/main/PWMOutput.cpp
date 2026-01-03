#include <Arduino.h>
#include "Log.h"
#include "IOT_Defines.h"
#include "PWMOutput.h"

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

