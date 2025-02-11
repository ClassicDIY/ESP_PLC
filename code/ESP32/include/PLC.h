#include <Arduino.h>
#include "IOTCallbackInterface.h"
#include <ModbusServerTCPasync.h>
#include "IOTServiceInterface.h"
#include "Defines.h"
#include "CoilData.h"
#include "AnalogSensor.h"
#include "DigitalSensor.h"
#include "Coil.h"

namespace ESP_PLC
{
	class PLC : public IOTCallbackInterface
	{
	
	public:
		PLC(WebSocketsServer* webSocket) : MBserver()
		{
			_pWebSocket = webSocket;
		}
		void setup(IOTServiceInterface* pcb);
		void Process();
		//IOTCallbackInterface 
		String getSettingsHTML() ;
		iotwebconf::ParameterGroup* parameterGroup() ;
		bool validate(iotwebconf::WebRequestWrapper* webRequestWrapper);
		void onMqttConnect(bool sessionPresent);
		void onMqttMessage(char* topic, JsonDocument& doc);
		void onWiFiConnect();
	
	protected:
		boolean PublishDiscoverySub(const char *component, const char *entityName, const char *jsonElement, const char *device_class, const char *unit_of_meas, const char *icon = "");
		bool ReadyToPublish() {
			return (!_discoveryPublished);
		}

	private:
		IOTServiceInterface* _iot;
		boolean _discoveryPublished = false;
		ModbusServerTCPasync MBserver;
		WebSocketsServer* _pWebSocket;
		String _lastMessagePublished;
		unsigned long _lastPublishTimeStamp = 0;

		Coil _Coils[DO_PINS] = {GPIO_NUM_26, GPIO_NUM_27, GPIO_NUM_32, GPIO_NUM_33};
		DigitalSensor _DigitalSensors[DI_PINS] = {GPIO_NUM_12, GPIO_NUM_13, GPIO_NUM_14, GPIO_NUM_15, GPIO_NUM_16, GPIO_NUM_17, GPIO_NUM_18, GPIO_NUM_19, GPIO_NUM_21, GPIO_NUM_22, GPIO_NUM_23, GPIO_NUM_25};
		AnalogSensor _AnalogSensors[AI_PINS] = {GPIO_NUM_34, GPIO_NUM_35, GPIO_NUM_36, GPIO_NUM_39};

		CoilData _digitalOutputCoils = CoilData(DO_PINS);
		CoilData _digitalInputDiscretes = CoilData(DI_PINS);
	};
}