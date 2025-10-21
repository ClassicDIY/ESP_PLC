#pragma once
#include "Arduino.h"
#include "ArduinoJson.h"

namespace ESP_PLC
{   
class IOTCallbackInterface
{
public:
    virtual void onMqttConnect() = 0;
    virtual void onMqttMessage(char* topic, char* payload) = 0;
    virtual void onNetworkConnect() = 0;
    virtual void addApplicationSettings(String& page);
    virtual void addApplicationConfigs(String& page);
    virtual void onSubmitForm(AsyncWebServerRequest *request);
    virtual void onSaveSetting(JsonDocument& doc);
    virtual void onLoadSetting(JsonDocument& doc);
    
};
} // namespace ESP_PLC