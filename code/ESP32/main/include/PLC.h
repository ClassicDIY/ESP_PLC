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
#include "Coil.h"
#include "IOTCallbackInterface.h"

namespace ESP_PLC
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
		void addApplicationSettings(String& page);
		void addApplicationConfigs(String& page);
		void onSubmitForm(AsyncWebServerRequest *request);
	    void onSaveSetting(JsonDocument& doc);
    	void onLoadSetting(JsonDocument& doc);

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
		#elif NORVI_GSM_AE02
		Coil _Coils[DO_PINS] = {DO0, DO1};
		DigitalSensor _DigitalSensors[DI_PINS] = {DI0, DI1, DI2, DI3, DI4, DI5, DI6, DI7};
		AnalogSensor _AnalogSensors[AI_PINS] = {AI0, AI1, AI2, AI3};
		#elif LILYGO_T_SIM7600G
		Coil _Coils[DO_PINS] = {DO0, DO1};
		DigitalSensor _DigitalSensors[DI_PINS] = {DI0, DI1};
		AnalogSensor _AnalogSensors[AI_PINS] = {};
		#endif
		CoilData _digitalOutputCoils = CoilData(DO_PINS);
		CoilData _digitalInputDiscretes = CoilData(DI_PINS);
		unsigned long _lastHeap = 0;
	};
}