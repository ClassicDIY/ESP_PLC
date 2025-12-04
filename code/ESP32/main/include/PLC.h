#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ModbusServerTCPasync.h>
#include "Defines.h"
#include "Device.h"

#include "IOTCallbackInterface.h"
#include "Enumerations.h"

namespace CLASSICDIY {
class PLC : public Device, public IOTCallbackInterface {
 public:
   PLC();
   ~PLC();
   void Setup();
   void CleanUp();
   void Process();
#ifdef HasMQTT
   void onMqttConnect();
   void onMqttMessage(char *topic, char *payload);
#endif
#ifdef HasModbus
   bool onModbusMessage(ModbusMessage &msg);
#endif
   void onNetworkState(NetworkState state);
   void onSaveSetting(JsonDocument &doc);
   void onLoadSetting(JsonDocument &doc);
   String appTemplateProcessor(const String &var);

 protected:
#ifdef HasMQTT
   boolean PublishDiscoverySub(IOTypes type, const char *entityName, const char *unit_of_meas = nullptr, const char *icon = nullptr);
#endif
 private:
   boolean _discoveryPublished = false;
   String _lastMessagePublished;
   unsigned long _lastModbusPollTime = 0;
   String _bodyBuffer;

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
} // namespace CLASSICDIY