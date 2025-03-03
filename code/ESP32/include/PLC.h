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
		void Process();
		void Monitor();
		void onMqttConnect(bool sessionPresent);
		void onMqttMessage(char* topic, JsonDocument& doc);
		void onWiFiConnect();
		void addNetworkSettings(String& page);
		void addNetworkConfigs(String& page);
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

		Coil _Coils[DO_PINS] = {GPIO_NUM_26, GPIO_NUM_27, GPIO_NUM_32, GPIO_NUM_33};
		DigitalSensor _DigitalSensors[DI_PINS] = {GPIO_NUM_12, GPIO_NUM_13, GPIO_NUM_14, GPIO_NUM_15, GPIO_NUM_16, GPIO_NUM_17, GPIO_NUM_18, GPIO_NUM_19, GPIO_NUM_21, GPIO_NUM_22, GPIO_NUM_23, GPIO_NUM_25};
		AnalogSensor _AnalogSensors[AI_PINS] = {GPIO_NUM_34, GPIO_NUM_35, GPIO_NUM_36, GPIO_NUM_39};

		CoilData _digitalOutputCoils = CoilData(DO_PINS);
		CoilData _digitalInputDiscretes = CoilData(DI_PINS);

		int16_t _digitalInputs = DI_PINS;
		int16_t _analogInputs = AI_PINS;
		unsigned long _lastHeap = 0;
	};
}