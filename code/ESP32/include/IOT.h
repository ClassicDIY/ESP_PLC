#pragma once
#include <Arduino.h>
#include "ArduinoJson.h"
#include <EEPROM.h>
#include <AsyncTCP.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <ModbusServerTCPasync.h>
#include "time.h"
#include <sstream>
#include <string>
#include <AsyncMqttClient.h>
#include "Defines.h"
#include "Enumerations.h"
#include "OTA.h"
#include "IOTServiceInterface.h"
#include "IOTCallbackInterface.h"

namespace ESP_PLC
{
    class IOT : public IOTServiceInterface
    {
    public:
        IOT() {};
        void Init(IOTCallbackInterface *iotCB, AsyncWebServer *pwebServer);

        void Run();
        boolean Publish(const char *subtopic, const char *value, boolean retained = false);
        boolean Publish(const char *subtopic, JsonDocument &payload, boolean retained = false);
        boolean Publish(const char *subtopic, float value, boolean retained = false);
        boolean PublishMessage(const char *topic, JsonDocument &payload, boolean retained);
        boolean PublishHADiscovery(JsonDocument &payload);
        std::string getRootTopicPrefix();
        u_int getUniqueId() { return _uniqueId; };
        std::string getThingName();
        void Online();
        IOTCallbackInterface *IOTCB() { return _iotCB; }
        void registerMBWorkers(FunctionCode fc, MBSworker worker);

    private:
        OTA _OTA = OTA();
        AsyncWebServer *_pwebServer;
        NetworkState _networkState = Boot;
        WiFiStatus _WiFiStatus;
        bool _blinkStateOn = false;
        String _AP_SSID = TAG;
        String _AP_Password = DEFAULT_AP_PASSWORD;
        String _SSID;
        String _WiFi_Password;
        bool _useMQTT = false;
        String _mqttServer;
        int16_t _mqttPort = 1883;
        String _mqttUserName;
        String _mqttUserPassword;
        bool _useModbus = false;
        int16_t _modbusPort = 502;
        int16_t _modbusID = 1;
        bool _clientsConfigured = false;
        IOTCallbackInterface *_iotCB;
        u_int _uniqueId = 0; // unique id from mac address NIC segment
        bool _publishedOnline = false;
        unsigned long _lastBlinkTime = 0;
        unsigned long _lastBootTimeStamp = millis();
        unsigned long _waitInAPTimeStamp = millis();
        unsigned long _wifiConnectionStart = 0;
        char _willTopic[STR_LEN];
        char _rootTopicPrefix[STR_LEN];
        void saveSettings();
        void loadSettings();
        void SendNetworkSettings(AsyncWebServerRequest *request);
        void ConnectToMQTTServer();
        void setState(NetworkState newState);
        static void mqttReconnectTimerCF(TimerHandle_t xTimer)
        {
            // Retrieve the instance of the class (stored as the timer's ID)
            IOT *instance = static_cast<IOT *>(pvTimerGetTimerID(xTimer));
            if (instance != nullptr)
            {
                instance->ConnectToMQTTServer();
            }
        }
    };

} // namespace ESP_PLC
