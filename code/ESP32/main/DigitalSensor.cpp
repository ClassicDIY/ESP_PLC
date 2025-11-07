#include <Arduino.h>
#include "Log.h"
#include "DigitalSensor.h"

namespace CLASSICDIY
{
	DigitalSensor::DigitalSensor(int sensorPin)
	{
		_sensorPin = sensorPin;
		pinMode(sensorPin, INPUT);
	}

	DigitalSensor::~DigitalSensor()
	{
	}

	std::string DigitalSensor::Pin()
	{
		std::stringstream ss;
		ss << "GPIO_" << _sensorPin;
		std::string formattedString = ss.str();
		return formattedString;
	}

	bool DigitalSensor::Level()
	{
		return (bool)digitalRead(_sensorPin);
	}

} // namespace namespace CLASSICDIY
