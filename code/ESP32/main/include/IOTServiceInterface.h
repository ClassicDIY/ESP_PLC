#pragma once
#include "Arduino.h"
#include "ArduinoJson.h"
#include "Enumerations.h"

namespace CLASSICDIY
{
class IOTServiceInterface
{
public:

    // MQTT related methods
    virtual boolean Publish(const char *subtopic, const char *value, boolean retained) = 0;
    virtual boolean Publish(const char *subtopic, float value, boolean retained) = 0;
    virtual boolean PublishMessage(const char* topic, JsonDocument& payload, boolean retained) = 0;
    virtual boolean PublishHADiscovery(JsonDocument& payload) = 0;
    virtual std::string getRootTopicPrefix() = 0;
    virtual u_int getUniqueId() = 0;
    virtual std::string getThingName() = 0;
    virtual void PublishOnline() = 0;

    // Modbus related methods
    virtual uint16_t getMBBaseAddress(IOTypes type) = 0;
    virtual boolean ModbusBridgeEnabled() = 0;
    virtual void registerMBTCPWorkers(FunctionCode fc, MBSworker worker) = 0;
    virtual Modbus::Error SendToModbusBridgeAsync(ModbusMessage& request);
    virtual ModbusMessage SendToModbusBridgeSync(ModbusMessage request);
};
} // namespace CLASSICDIY