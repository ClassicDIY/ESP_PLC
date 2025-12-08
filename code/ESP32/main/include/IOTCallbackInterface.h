#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#ifdef HasModbus
#include <ModbusServerTCPasync.h>
#endif
#include "Enumerations.h"

namespace CLASSICDIY {
class IOTCallbackInterface {
 public:
   virtual void onNetworkState(NetworkState state) = 0;
   virtual String appTemplateProcessor(const String &var);
   virtual void onSaveSetting(JsonDocument &doc);
   virtual void onLoadSetting(JsonDocument &doc);
#ifdef HasMQTT
   virtual void onMqttConnect() = 0;
   virtual void onMqttMessage(char *topic, char *payload) = 0;
#endif
#if defined(HasModbus) && defined(HasRS485)
   virtual bool onModbusMessage(ModbusMessage &msg);
#endif
#ifdef Has_OLED
   virtual void update(const char *mode, const char *detail);
   virtual void update(const char *mode, int count);
#endif
};
} // namespace CLASSICDIY