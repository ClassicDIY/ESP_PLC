#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

namespace CLASSICDIY {
class IOTCallbackInterface {
 public:
   virtual void onNetworkConnect() = 0;
   virtual void addApplicationConfigs(String &page);
   virtual void onSubmitForm(AsyncWebServerRequest *request);
   virtual void onSaveSetting(JsonDocument &doc);
   virtual void onLoadSetting(JsonDocument &doc);
#ifdef HasMQTT
   virtual void onMqttConnect() = 0;
   virtual void onMqttMessage(char *topic, char *payload) = 0;
#endif
#ifdef HasModbus
   virtual bool onModbusMessage(ModbusMessage &msg);
#endif
};
} // namespace CLASSICDIY