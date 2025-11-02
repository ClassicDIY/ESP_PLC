#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <ModbusServerTCPasync.h>
#include "Defines.h"
#include "CoilData.h"
#include "AnalogSensor.h"
#include "DigitalSensor.h"
#include "PWMOutput.h"
#include "Coil.h"
#include "IOTCallbackInterface.h"

namespace CLASSICDIY
{
	class PLC : public IOTCallbackInterface
	{
	public:
		PLC() {};
		void setup();
		void CleanUp();
		void Monitor();
		void Process();
		void onMqttConnect();
		void onMqttMessage(char* topic, char *payload);
		void onNetworkConnect();
		void addApplicationConfigs(String& page);
		void onSubmitForm(AsyncWebServerRequest *request);
	    void onSaveSetting(JsonDocument& doc);
    	void onLoadSetting(JsonDocument& doc);
		bool onModbusMessage(ModbusMessage& msg);

	protected:
		boolean PublishDiscoverySub(const char *component, const char *entityName, const char *jsonElement, const char *device_class, const char *unit_of_meas, const char *icon = "");
		bool ReadyToPublish() {
			return (!_discoveryPublished);
		}

	private:
		boolean _discoveryPublished = false;
		String _lastMessagePublished;
		unsigned long _lastPublishTimeStamp = 0;

		#ifdef EDGEBOX
		Coil _Coils[DO_PINS] = {DO0, DO1, DO2, DO3, DO4, DO5};
		DigitalSensor _DigitalSensors[DI_PINS] = {DI0, DI1, DI2, DI3};
		AnalogSensor _AnalogSensors[AI_PINS] = {AI0, AI1, AI2, AI3};
		PWMOutput _PWMOutputs[AO_PINS] = {AO0, AO1, AO2, AO3};
		#elif NORVI_GSM_AE02
		Coil _Coils[DO_PINS] = {DO0, DO1};
		DigitalSensor _DigitalSensors[DI_PINS] = {DI0, DI1, DI2, DI3, DI4, DI5, DI6, DI7};
		AnalogSensor _AnalogSensors[AI_PINS] = {AI0, AI1, AI2, AI3};
		#elif LILYGO_T_SIM7600G
		Coil _Coils[DO_PINS] = {DO0, DO1};
		DigitalSensor _DigitalSensors[DI_PINS] = {DI0, DI1};
		AnalogSensor _AnalogSensors[AI_PINS] = {};
		#elif Waveshare_Relay_6CH
		Coil _Coils[DO_PINS] = {DO0, DO1, DO2, DO3, DO4, DO5};
		DigitalSensor _DigitalSensors[DI_PINS] = {};
		AnalogSensor _AnalogSensors[AI_PINS] = {};
		PWMOutput _PWMOutputs[AO_PINS] = {AO0, AO1, AO2, AO3};
		#endif

		CoilData _digitalOutputCoils;
		CoilData _digitalInputDiscretes;
		uint8_t _analogInputCount;
		uint8_t _analogOutputCount;

		// Modbus Bridge settings
		uint8_t _inputID = 0;
        uint16_t _inputAddress = 0;
        uint8_t _inputCount = 0;

        uint8_t _coilID = 0;
        uint16_t _coilAddress = 0;
        uint8_t _coilCount = 0;

        uint8_t _discreteID = 0;
        uint16_t _discreteAddress = 0;
        uint8_t _discreteCount = 0;

        uint8_t _holdingID = 0;
        uint16_t _holdingAddress = 0;
        uint8_t _holdingCount = 0;
	};
}