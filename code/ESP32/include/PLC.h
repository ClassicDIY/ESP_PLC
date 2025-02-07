#include "IOTCallbackInterface.h"
#include <ModbusServerTCPasync.h>
#include "IOTServiceInterface.h"
#include "Defines.h"
#include "CoilData.h"

namespace ESP_PLC
{
	class PLC : public IOTCallbackInterface
	{
	
	public:
		PLC();
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
		String _lastMessagePublished;
		uint16_t AddReading(uint16_t val);
		uint16_t _rollingSum;
		uint16_t _numberOfSummations;
		uint16_t _count;
		CoilData _digitalOutputCoils = CoilData(DO_PINS);
		CoilData _digitalInputDiscretes = CoilData(DI_PINS);
	};
}