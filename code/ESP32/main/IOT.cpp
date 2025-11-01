#include <sys/time.h>
#include <thread>
#include <chrono>
#include <ESPmDNS.h>
#include <SPI.h>
#include <Ethernet.h>
#include <esp_netif.h>
#include <esp_eth.h>
#include <esp_event.h>
#include "esp_mac.h"
#include "esp_eth_mac.h"
#include "esp_netif_ppp.h"
#include "driver/spi_master.h"
#ifdef HasLTE
#include "network_dce.h"
#endif
#include "Log.h"
#include "WebLog.h"
#include "IOT.h"
#include "style.html"
#include "IOT.html"
#include "HelperFunctions.h"

#ifdef Has_OLED_Display
#include <Adafruit_GFX.h> 
#include <Adafruit_SSD1306.h>

extern Adafruit_SSD1306 oled_display;

#endif

namespace CLASSICDIY
{
	TimerHandle_t mqttReconnectTimer;
	static DNSServer _dnsServer;
	static WebLog _webLog;
	static ModbusServerTCPasync _MBserver;
	static ModbusServerRTU _MBRTUserver(2000);
	#ifdef HasRS485
	ModbusClientRTU _MBclientRTU(RS485_RTS, MODBUS_RTU_REQUEST_QUEUE_SIZE);
	#endif
	static AsyncAuthenticationMiddleware basicAuth;

// #pragma region Setup
	void IOT::Init(IOTCallbackInterface *iotCB, AsyncWebServer *pwebServer)
	{
		_iotCB = iotCB;
		_pwebServer = pwebServer;
#ifdef WIFI_STATUS_PIN
		pinMode(WIFI_STATUS_PIN, OUTPUT); // use LED for wifi AP status (note:edgeBox shares the LED pin with the serial TX gpio)
#endif
		#ifdef FACTORY_RESET_PIN // use digital input pin for factory reset
		pinMode(FACTORY_RESET_PIN, INPUT_PULLUP);
		EEPROM.begin(EEPROM_SIZE);
		if (digitalRead(FACTORY_RESET_PIN) == LOW)
		{
			logi("Factory Reset");
			EEPROM.write(0, 0);
			EEPROM.commit();
		}
		#else // use analog pin for factory reset
		EEPROM.begin(EEPROM_SIZE);
		uint16_t analogValue = analogRead(BUTTONS);
		logd("button value (%d)", analogValue);
		if (analogValue > 3000)
		{
			logi("**********************Factory Reset*************************(%d)", analogValue);
			EEPROM.write(0, 0);
			EEPROM.commit();
			saveSettings();
		}
		#endif
		else
		{
			logi("Loading configuration from EEPROM");
			loadSettings();
		}
		#ifdef HasRS485
		if (RS485_RTS != -1)
		{
			pinMode(RS485_RTS, OUTPUT);
		}
		if (_ModbusMode == RTU)
		{
			// Set up Serial2 connected to Modbus RTU server
			RTUutils::prepareHardwareSerial(Serial2);
			SerialConfig conf = getSerialConfig(_modbusParity, _modbusStopBits);
			logd("Serial baud: %d conf: 0x%x", _modbusBaudRate, conf);
			Serial2.begin(_modbusBaudRate, conf, RS485_RXD, RS485_TXD);
			while (!Serial2) {}
		} 
		else if (_useModbusBridge)
		{
			// Set up Serial2 connected to Modbus RTU Client
			RTUutils::prepareHardwareSerial(Serial2);
			SerialConfig conf = getSerialConfig(_modbusClientParity, _modbusClientStopBits);
			logd("Serial baud: %d conf: 0x%x", _modbusClientBaudRate, conf);
			Serial2.begin(_modbusClientBaudRate, conf, RS485_RXD, RS485_TXD);
			while (!Serial2) {}
		}
		#endif
		mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(8000), pdFALSE, this, mqttReconnectTimerCF);

		WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info)
		{
			String s;
			JsonDocument doc;
			switch (event)
			{
			case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
				logd("AP_STADISCONNECTED");
				_AP_Connected = false;
				GoOffline();
			break;
			case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
				logd("AP_STAIPASSIGNED");
				sprintf(_Current_IP, "%s", WiFi.softAPIP().toString().c_str());
				logd("Current_IP: %s", _Current_IP);
				_AP_Connected = true;
				GoOnline();
			break;
			case ARDUINO_EVENT_WIFI_STA_GOT_IP:
				logd("STA_GOT_IP");
				doc["IP"] = WiFi.localIP().toString().c_str();
				sprintf(_Current_IP, "%s", WiFi.localIP().toString().c_str());
				logi("Got IP Address");
				logi("~~~~~~~~~~~");
				logi("IP: %s", _Current_IP);
				logi("IPMASK: %s", WiFi.subnetMask().toString().c_str());
				logi("Gateway: %s", WiFi.gatewayIP().toString().c_str());
				logi("~~~~~~~~~~~");
				doc["ApPassword"] = DEFAULT_AP_PASSWORD;
				serializeJson(doc, s);
				s += '\n';
				Serial.printf(s.c_str()); // send json to flash tool
				configTime(0, 0, NTP_SERVER);
				printLocalTime();
				GoOnline();
				break;
			case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
				logw("STA_DISCONNECTED");
				GoOffline();
				break;
			default:
				logd("[WiFi-event] event: %d", event);
				break;
			} 
		});
		// generate unique id from mac address NIC segment
		uint8_t chipid[6];
		esp_efuse_mac_get_default(chipid);
		_uniqueId = chipid[3] << 16;
		_uniqueId += chipid[4] << 8;
		_uniqueId += chipid[5];
		_lastBootTimeStamp = millis();
		_pwebServer->on("/reboot", [this](AsyncWebServerRequest *request)
		{ 
			logd("resetModule");
			String page = reboot_html;
			request->send(200, "text/html", page.c_str());
			delay(3000);
			esp_restart(); 
		});
		_pwebServer->onNotFound([this](AsyncWebServerRequest *request)
		{
			RedirectToHome(request); 
		});
		basicAuth.setUsername("admin");
		basicAuth.setPassword(_AP_Password.c_str());
		basicAuth.setAuthFailureMessage("Authentication failed");
		basicAuth.setAuthType(AsyncAuthType::AUTH_BASIC);
		basicAuth.generateHash();
		_pwebServer->on("/settings", HTTP_GET, [this](AsyncWebServerRequest *request)
		{
			String fields = network_config_fields;
			fields.replace("{n}", _AP_SSID);
			fields.replace("{v}", APP_VERSION);
			fields.replace("{AP_SSID}", _AP_SSID);
			fields.replace("{AP_Pw}", _AP_Password);
			fields.replace("{WIFI}", _NetworkSelection == WiFiMode ? "selected" : "");
			#ifdef HasEthernet
			fields.replace("{ETH}", _NetworkSelection == EthernetMode ? "selected" : "");
			#else
			fields.replace("{ETH}", "class='hidden'");
			#endif
			fields.replace("{4G}", _NetworkSelection == ModemMode ? "selected" : "");
			fields.replace("{SSID}", _SSID);
			fields.replace("{WiFi_Pw}", _WiFi_Password);
			fields.replace("{dhcpChecked}", _useDHCP ? "checked" : "unchecked");
			fields.replace("{ETH_SIP}", _Static_IP);
			fields.replace("{ETH_SM}", _Subnet_Mask);
			fields.replace("{ETH_GW}", _Gateway_IP);
			fields.replace("{APN}", _APN);
			fields.replace("{SIM_USERNAME}", _SIM_Username);
			fields.replace("{SIM_PASSWORD}", _SIM_Password);
			fields.replace("{SIM_PIN}", _SIM_PIN);
			fields.replace("{mqttchecked}", _useMQTT ? "checked" : "unchecked");
			fields.replace("{mqttServer}", _mqttServer);
			fields.replace("{mqttPort}", String(_mqttPort));
			fields.replace("{mqttUser}", _mqttUserName);
			fields.replace("{mqttPw}", _mqttUserPassword);
			fields.replace("{modbuschecked}", _useModbus ? "checked" : "unchecked");
			fields.replace("{TCP}", _ModbusMode == TCP ? "selected" : "");
			fields.replace("{RTU}", _ModbusMode == RTU ? "selected" : "");
			fields.replace("{RTU_SVR_9600}", _modbusBaudRate == 9600 ? "selected" : "");
			fields.replace("{RTU_SVR_19200}", _modbusBaudRate == 19200 ? "selected" : "");
			fields.replace("{RTU_SVR_38400}", _modbusBaudRate == 38400 ? "selected" : "");
			fields.replace("{RTU_SVR_115200}", _modbusBaudRate == 115200 ? "selected" : "");
			fields.replace("{RTU_SVR_Parity_None}", _modbusParity == UART_PARITY_DISABLE ? "selected" : "");
			fields.replace("{RTU_SVR_Parity_Even}", _modbusParity == UART_PARITY_EVEN ? "selected" : "");
			fields.replace("{RTU_SVR_Parity_Odd}", _modbusParity == UART_PARITY_ODD ? "selected" : "");
			fields.replace("{RTU_SVR_1Stop}", _modbusStopBits == UART_STOP_BITS_1 ? "selected" : "");
			fields.replace("{RTU_SVR_2Stop}", _modbusStopBits == UART_STOP_BITS_2 ? "selected" : "");
			fields.replace("{modbusPort}", String(_modbusPort));
			fields.replace("{modbusID}", String(_modbusID));
			fields.replace("{inputRegBase}", String(_input_register_base_addr));
			fields.replace("{coilBase}", String(_coil_base_addr));
			fields.replace("{discreteBase}", String(_discrete_input_base_addr));
			fields.replace("{holdingRegBase}", String(_holding_register_base_addr));
			fields.replace("{modbusBridgechecked}", _useModbusBridge ? "checked" : "unchecked");
			fields.replace("{RTU_CLIENT_9600}", _modbusClientBaudRate == 9600 ? "selected" : "");
			fields.replace("{RTU_CLIENT_19200}", _modbusClientBaudRate == 19200 ? "selected" : "");
			fields.replace("{RTU_CLIENT_38400}", _modbusClientBaudRate == 38400 ? "selected" : "");
			fields.replace("{RTU_CLIENT_115200}", _modbusClientBaudRate == 115200 ? "selected" : "");
			fields.replace("{RTU_CLIENT_Parity_None}", _modbusClientParity == UART_PARITY_DISABLE ? "selected" : "");
			fields.replace("{RTU_CLIENT_Parity_Even}", _modbusClientParity == UART_PARITY_EVEN ? "selected" : "");
			fields.replace("{RTU_CLIENT_Parity_Odd}", _modbusClientParity == UART_PARITY_ODD ? "selected" : "");
			fields.replace("{RTU_CLIENT_1Stop}", _modbusClientStopBits == UART_STOP_BITS_1 ? "selected" : "");
			fields.replace("{RTU_CLIENT_2Stop}", _modbusClientStopBits == UART_STOP_BITS_2 ? "selected" : "");
			String page = network_config_top;
			page.replace("{style}", style);
			page.replace("{n}", _AP_SSID);
			page.replace("{v}", APP_VERSION);
			page += fields;
			_iotCB->addApplicationConfigs(page);
			String apply_button = network_config_apply_button;
			page += apply_button;
			#ifdef HasOTA
			page +=  network_config_links;
			#else 
			page += network_config_links_no_ota; 
			#endif
			request->send(200, "text/html", page); 
		}).addMiddleware(&basicAuth);

		_pwebServer->on("/submit", HTTP_POST, [this](AsyncWebServerRequest *request)
						{
			logd("submit");
			if (request->hasParam("AP_SSID", true)) {
				_AP_SSID = request->getParam("AP_SSID", true)->value().c_str();
			}
			if (request->hasParam("AP_Pw", true)) {
				_AP_Password = request->getParam("AP_Pw", true)->value().c_str();
			}
			if (request->hasParam("SSID", true)) {
				_SSID = request->getParam("SSID", true)->value().c_str();
			}
			if (request->hasParam("networkSelector", true)) {
				String sel =  request->getParam("networkSelector", true)->value();
				_NetworkSelection = sel == "wifi" ? WiFiMode : sel == "ethernet" ? EthernetMode : ModemMode;
			}
			if (request->hasParam("WiFi_Pw", true)) {
				_WiFi_Password = request->getParam("WiFi_Pw", true)->value().c_str();
			}
			if (request->hasParam("APN", true)) {
				_APN = request->getParam("APN", true)->value().c_str();
			}
			if (request->hasParam("SIM_USERNAME", true)) {
				_SIM_Username = request->getParam("SIM_USERNAME", true)->value().c_str();
			}
			if (request->hasParam("SIM_PASSWORD", true)) {
				_SIM_Password = request->getParam("SIM_PASSWORD", true)->value().c_str();
			}
			if (request->hasParam("SIM_PIN", true)) {
				_SIM_PIN = request->getParam("SIM_PIN", true)->value().c_str();
			}
			#ifdef HasEthernet
			_useDHCP =  request->hasParam("dhcpCheckbox", true);
			if (request->hasParam("ETH_SIP", true)) {
				_Static_IP = request->getParam("ETH_SIP", true)->value().c_str();
			}
			if (request->hasParam("ETH_SM", true)) {
				_Subnet_Mask = request->getParam("ETH_SM", true)->value().c_str();
			}
			if (request->hasParam("ETH_GW", true)) {
				_Gateway_IP = request->getParam("ETH_GW", true)->value().c_str();
			}
			#endif
			_useMQTT =  request->hasParam("mqttCheckbox", true);
			if (request->hasParam("mqttServer", true)) {
				_mqttServer = request->getParam("mqttServer", true)->value().c_str();
			}
			if (request->hasParam("mqttPort", true)) {
				_mqttPort = request->getParam("mqttPort", true)->value().toInt();
			}
			if (request->hasParam("mqttUser", true)) {
				_mqttUserName = request->getParam("mqttUser", true)->value().c_str();
			}
			if (request->hasParam("mqttPw", true)) {
				_mqttUserPassword = request->getParam("mqttPw", true)->value().c_str();
			}
			_useModbus = request->hasParam("modbusCheckbox", true);
			if (request->hasParam("modbusModeSelector", true)) {
				String sel =  request->getParam("modbusModeSelector", true)->value();
				_ModbusMode = sel == "tcp" ? TCP : RTU;
			}
			if (request->hasParam("svrRTUBaud", true)) {
				_modbusBaudRate =  request->getParam("svrRTUBaud", true)->value().toInt();
			}
			if (request->hasParam("svrRTUParity", true)) {
				String sel =  request->getParam("svrRTUParity", true)->value().c_str();
				_modbusParity = sel == "none" ? UART_PARITY_DISABLE : sel == "even" ? UART_PARITY_EVEN : UART_PARITY_ODD;
			}
			if (request->hasParam("svrRTUStopBits", true)) {
				String sel =  request->getParam("svrRTUStopBits", true)->value().c_str();
				_modbusStopBits = sel == "1" ? UART_STOP_BITS_1 : UART_STOP_BITS_2;
			}
			if (request->hasParam("modbusPort", true)) {
				_modbusPort = request->getParam("modbusPort", true)->value().toInt();
			}
			if (request->hasParam("modbusID", true)) {
				_modbusID = request->getParam("modbusID", true)->value().toInt();
			}
			if (request->hasParam("inputRegBase", true)) {
				_input_register_base_addr = request->getParam("inputRegBase", true)->value().toInt();
			}
			if (request->hasParam("coilBase", true)) {
				_coil_base_addr = request->getParam("coilBase", true)->value().toInt();
			}
			if (request->hasParam("discreteBase", true)) {
				_discrete_input_base_addr = request->getParam("discreteBase", true)->value().toInt();
			}
			if (request->hasParam("holdingRegBase", true)) {
				_holding_register_base_addr = request->getParam("holdingRegBase", true)->value().toInt();
			}
			_useModbusBridge = request->hasParam("modbusBridgeCheckbox", true);
			if (request->hasParam("clientRTUBaud", true)) {
				_modbusClientBaudRate =  request->getParam("clientRTUBaud", true)->value().toInt();
			}
			if (request->hasParam("clientRTUParity", true)) {
				String sel =  request->getParam("clientRTUParity", true)->value().c_str();
				_modbusClientParity = sel == "none" ? UART_PARITY_DISABLE : sel == "even" ? UART_PARITY_EVEN : UART_PARITY_ODD;
			}
			if (request->hasParam("clientRTUStopBits", true)) {
				String sel =  request->getParam("clientRTUStopBits", true)->value().c_str();
				_modbusClientStopBits = sel == "1" ? UART_STOP_BITS_1 : UART_STOP_BITS_2;
			}
			_iotCB->onSubmitForm(request);
			saveSettings(); 
			RedirectToHome(request);
		});
	}

	void IOT::RedirectToHome(AsyncWebServerRequest* request)
	{
		logd("Redirecting from: %s", request->url().c_str());
		String page = redirect_html;
		page.replace("{n}", _SSID);
		page.replace("{ip}", _Current_IP);
		request->send(200, "text/html", page);
	}

	void IOT::loadSettings()
	{
		String jsonString;
		char ch;
		for (int i = 0; i < EEPROM_SIZE; ++i)
		{
			ch = EEPROM.read(i);
			if (ch == '\0')
				break; // Stop at the null terminator
			jsonString += ch;
		}
		logd("Settings JSON: " );
		ets_printf("%s\r\n", jsonString.c_str());
		JsonDocument doc;
		DeserializationError error = deserializeJson(doc, jsonString);
		if (error)
		{
			loge("Failed to load data from EEPROM, using defaults: %s", error.c_str());
			saveSettings(); // save default values
		}
		else
		{
			logd("JSON loaded from EEPROM: %d", jsonString.length());
			JsonObject iot = doc["iot"].as<JsonObject>();
			_AP_SSID = iot["AP_SSID"].isNull() ? TAG : iot["AP_SSID"].as<String>();
			_AP_Password = iot["AP_Pw"].isNull() ? DEFAULT_AP_PASSWORD : iot["AP_Pw"].as<String>();
			_NetworkSelection = iot["Network"].isNull() ? WiFiMode : iot["Network"].as<NetworkSelection>();
			_SSID = iot["SSID"].isNull() ? "" : iot["SSID"].as<String>();
			_WiFi_Password = iot["WiFi_Pw"].isNull() ? "" : iot["WiFi_Pw"].as<String>();
			_APN = iot["APN"].isNull() ? "" : iot["APN"].as<String>();
			_SIM_Username = iot["SIM_USERNAME"].isNull() ? "" : iot["SIM_USERNAME"].as<String>();
			_SIM_Password = iot["SIM_PASSWORD"].isNull() ? "" : iot["SIM_PASSWORD"].as<String>();
			_SIM_PIN = iot["SIM_PIN"].isNull() ? "" : iot["SIM_PIN"].as<String>();
			_useDHCP = iot["useDHCP"].isNull() ? false : iot["useDHCP"].as<bool>();
			_Static_IP = iot["ETH_SIP"].isNull() ? "" : iot["ETH_SIP"].as<String>();
			_Subnet_Mask = iot["ETH_SM"].isNull() ? "" : iot["ETH_SM"].as<String>();
			_Gateway_IP = iot["ETH_GW"].isNull() ? "" : iot["ETH_GW"].as<String>();
			_useMQTT = iot["useMQTT"].isNull() ? false : iot["useMQTT"].as<bool>();
			_mqttServer = iot["mqttServer"].isNull() ? "" : iot["mqttServer"].as<String>();
			_mqttPort = iot["mqttPort"].isNull() ? 1883 : iot["mqttPort"].as<uint16_t>();
			_mqttUserName = iot["mqttUser"].isNull() ? "" : iot["mqttUser"].as<String>();
			_mqttUserPassword = iot["mqttPw"].isNull() ? "" : iot["mqttPw"].as<String>();
			_useModbus = iot["useModbus"].isNull() ? false : iot["useModbus"].as<bool>();
			_ModbusMode = iot["modbusMode"].isNull() ? TCP : iot["modbusMode"].as<ModbusMode>();
			_modbusBaudRate = iot["svrRTUBaud"].isNull() ? 9600 : iot["svrRTUBaud"].as<uint32_t>();
			_modbusParity = iot["svrRTUParity"].isNull() ? UART_PARITY_DISABLE : iot["svrRTUParity"].as<uart_parity_t>();
			_modbusStopBits = iot["svrRTUStopBits"].isNull() ? UART_STOP_BITS_1 : iot["svrRTUStopBits"].as<uart_stop_bits_t>();
			_modbusPort = iot["modbusPort"].isNull() ? 502 : iot["modbusPort"].as<uint16_t>();
			_modbusID = iot["modbusID"].isNull() ? 1 : iot["modbusID"].as<uint16_t>();
			_input_register_base_addr = iot["inputRegBase"].isNull() ? INPUT_REGISTER_BASE_ADDRESS : iot["inputRegBase"].as<uint16_t>();
			_coil_base_addr = iot["coilBase"].isNull() ? COIL_BASE_ADDRESS : iot["coilBase"].as<uint16_t>();
			_discrete_input_base_addr = iot["discreteBase"].isNull() ? DISCRETE_BASE_ADDRESS : iot["discreteBase"].as<uint16_t>();
			_holding_register_base_addr = iot["holdingRegBase"].isNull() ? HOLDING_REGISTER_BASE_ADDRESS : iot["holdingRegBase"].as<uint16_t>();
			_useModbusBridge = iot["useModbusBridge"].isNull() ? false : iot["useModbusBridge"].as<bool>();
			_modbusClientBaudRate = iot["modbusClientBaudRate"].isNull() ? 9600 : iot["modbusClientBaudRate"].as<uint32_t>();
			_modbusClientParity = iot["modbusClientParity"].isNull() ? UART_PARITY_DISABLE : iot["modbusClientParity"].as<uart_parity_t>();
			_modbusClientStopBits = iot["modbusClientStopBits"].isNull() ? UART_STOP_BITS_1 : iot["modbusClientStopBits"].as<uart_stop_bits_t>();
			_iotCB->onLoadSetting(doc);
		}
	}

	void IOT::saveSettings()
	{
		JsonDocument doc;
		JsonObject iot = doc["iot"].to<JsonObject>();
		iot["version"] = APP_VERSION;
		iot["AP_SSID"] = _AP_SSID;
		iot["AP_Pw"] = _AP_Password;
		iot["Network"] = _NetworkSelection;
		iot["SSID"] = _SSID;
		iot["WiFi_Pw"] = _WiFi_Password;
		iot["APN"] = _APN;
		iot["SIM_USERNAME"] = _SIM_Username;
		iot["SIM_PASSWORD"] = _SIM_Password;
		iot["SIM_PIN"] = _SIM_PIN;
		iot["useDHCP"] = _useDHCP;
		iot["ETH_SIP"] = _Static_IP;
		iot["ETH_SM"] = _Subnet_Mask;
		iot["ETH_GW"] = _Gateway_IP;
		iot["useMQTT"] = _useMQTT;
		iot["mqttServer"] = _mqttServer;
		iot["mqttPort"] = _mqttPort;
		iot["mqttUser"] = _mqttUserName;
		iot["mqttPw"] = _mqttUserPassword;
		iot["useModbus"] = _useModbus;
		iot["modbusMode"] = _ModbusMode;
		iot["svrRTUBaud"] = _modbusBaudRate;
		iot["svrRTUParity"] = _modbusParity;
		iot["svrRTUStopBits"] = _modbusStopBits;
		iot["modbusPort"] = _modbusPort;
		iot["modbusID"] = _modbusID;
		iot["inputRegBase"] = _input_register_base_addr;
		iot["coilBase"] = _coil_base_addr;
		iot["discreteBase"] = _discrete_input_base_addr;
		iot["holdingRegBase"] = _holding_register_base_addr;
		iot["useModbusBridge"] = _useModbusBridge;
		iot["modbusClientBaudRate"] = _modbusClientBaudRate;
		iot["modbusClientParity"] = _modbusClientParity;
		iot["modbusClientStopBits"] = _modbusClientStopBits;
		_iotCB->onSaveSetting(doc);
		String jsonString;
		serializeJson(doc, jsonString);
		// Serial.println(jsonString.c_str());
		for (int i = 0; i < jsonString.length(); ++i)
		{
			EEPROM.write(i, jsonString[i]);
		}
		EEPROM.write(jsonString.length(), '\0'); // Null-terminate the string
		EEPROM.commit();
		logd("JSON saved, required EEPROM size: %d", jsonString.length());
	}

	void IOT::Run()
	{
		uint32_t now = millis();
		if (_networkState == Boot && _NetworkSelection == NotConnected)
		{ // Network not setup?, see if flasher is trying to send us the SSID/Pw
			if (Serial.peek() == '{')
			{
				String s = Serial.readStringUntil('}');
				s += "}";
				JsonDocument doc;
				DeserializationError err = deserializeJson(doc, s);
				if (err)
				{
					loge("deserializeJson() failed: %s", err.c_str());
				}
				else
				{
					if (doc["ssid"].is<const char *>() && doc["password"].is<const char *>())
					{
						_SSID = doc["ssid"].as<String>();
						logd("Setting ssid: %s", _SSID.c_str());
						_WiFi_Password = doc["password"].as<String>();
						logd("Setting password: %s", _WiFi_Password.c_str());
						_NetworkSelection = WiFiMode;
						saveSettings();
						esp_restart();
					}
					else
					{
						logw("Received invalid json: %s", s.c_str());
					}
				}
			}
			else
			{
				Serial.read(); // discard data
			}
			if ((now - _FlasherIPConfigStart) > FLASHER_TIMEOUT) // wait for flasher tool to send Wifi info
			{
				logd("Done waiting for flasher!");
				setState(ApState); // switch to AP mode for AP_TIMEOUT
			}
		}
		else if (_networkState == Boot) // have network selection, start with wifiAP for AP_TIMEOUT then STA mode
		{
			setState(ApState); // switch to AP mode for AP_TIMEOUT
		}
		else if (_networkState == ApState)
		{
			if (_AP_Connected == false) // if AP client is connected, stay in AP mode
			{
				if ((now - _waitInAPTimeStamp) > AP_TIMEOUT) // switch to selected network after waiting in APMode for AP_TIMEOUT duration
				{
					logd("Connecting to network: %d", _NetworkSelection);
					setState(Connecting);
				}
				else
				{
					UpdateOledDisplay(); // update countdown
				}
			}
			_dnsServer.processNextRequest();
			_webLog.process();
		}
		else if (_networkState == Connecting)
		{
			if ((millis() - _NetworkConnectionStart) > WIFI_CONNECTION_TIMEOUT)
			{
				// -- Network not available, fall back to AP mode.
				logw("Giving up on Network connection.");
				WiFi.disconnect(true);
				setState(ApState);
			}
		}
		else if (_networkState == OffLine) // went offline, try again...
		{
			logw("went offline, try again...");
			setState(Connecting);
		}
		else if (_networkState == OnLine)
		{
			_webLog.process();
		}
#ifdef WIFI_STATUS_PIN
		// use LED if the log level is none (edgeBox shares the LED pin with the serial TX gpio)
		// handle blink led, fast : NotConnected slow: AP connected On: Station connected
		if (_networkState != OnLine)
		{
			unsigned long binkRate = _networkState == ApState ? AP_BLINK_RATE : NC_BLINK_RATE;
			unsigned long now = millis();
			if (binkRate < now - _lastBlinkTime)
			{
				_blinkStateOn = !_blinkStateOn;
				_lastBlinkTime = now;
				digitalWrite(WIFI_STATUS_PIN, _blinkStateOn ? HIGH : LOW);
			}
		}
		else
		{
			digitalWrite(WIFI_STATUS_PIN, HIGH);
		}
#elif RGB_LED_PIN
		if (_networkState != OnLine)
		{
			unsigned long binkRate = _networkState == ApState ? AP_BLINK_RATE : NC_BLINK_RATE;
			unsigned long now = millis();
			if (binkRate < now - _lastBlinkTime)
			{
				_blinkStateOn = !_blinkStateOn;
				_lastBlinkTime = now;
				RGB_Light(_blinkStateOn ? 60 : 0, _blinkStateOn ? 0 : 60, 0);
			}
		}
		else
		{
			RGB_Light(0, 0, 60);
		}
#endif

		vTaskDelay(pdMS_TO_TICKS(20));
		return;
	}

	void IOT::UpdateOledDisplay()
	{
#ifdef Has_OLED_Display
		oled_display.clearDisplay();
		oled_display.setTextSize(2);
		oled_display.setTextColor(SSD1306_WHITE);
		oled_display.setCursor(0,0);
		oled_display.println("ESP_PLC");
		oled_display.setTextSize(1);
		oled_display.println(APP_VERSION);
		oled_display.setTextSize(2);
		oled_display.setCursor(0, 30);
		
		if (_networkState == OnLine)
		{
			oled_display.println(_NetworkSelection == WiFiMode ? "WiFi: " : "LTE: ");
			oled_display.setTextSize(1);
			oled_display.println(_Current_IP);
		}
		else if (_networkState == Connecting)
		{
			oled_display.println("Connecting...");
		}
		else if (_networkState == ApState)
		{
			oled_display.println("AP Mode");
			int countdown = (AP_TIMEOUT - (millis() - _waitInAPTimeStamp)) / 1000;
			if (countdown > 0)
			{
				oled_display.setTextSize(2);
				oled_display.printf("%d", countdown);
			}
		}
		else
		{
			oled_display.println("Offline");
		}
		oled_display.display();
#endif
	}

// #pragma endregion Setup

// #pragma region Network

	void IOT::GoOnline()
	{
		logd("GoOnline called");
		_pwebServer->begin();
		_webLog.begin(_pwebServer);
		#ifdef HasOTA
		_OTA.begin(_pwebServer);
		#endif
		if (_networkState > ApState)
		{
			if (_NetworkSelection == EthernetMode || _NetworkSelection == WiFiMode)
			{
				MDNS.begin(_AP_SSID.c_str());
				MDNS.addService("http", "tcp", ASYNC_WEBSERVER_PORT);
				logd("Active mDNS services: %d", MDNS.queryService("http", "tcp"));
			}
			_iotCB->onNetworkConnect();
			if (_useModbus && !_MBserver.isRunning())
			{
				if (_ModbusMode == TCP)
				{
					if (!_MBserver.isRunning())
					{
						_MBserver.start(_modbusPort, 5, 0); // listen for modbus requests
						logd("Modbus TCP started");
					}
					#ifdef HasRS485
					if (ModbusBridgeEnabled())
					{
						_MBclientRTU.setTimeout(2000);
						_MBclientRTU.begin(Serial2);
						_MBclientRTU.useModbusRTU();
						_MBclientRTU.onDataHandler([this](ModbusMessage response, uint32_t token)
						{
							logv("RTU Response: serverID=%d, FC=%d, Token=%08X, length=%d:\n", response.getServerID(), response.getFunctionCode(), token, response.size());
							_iotCB->onModbusMessage(response);
							return true;
						});
						_MBclientRTU.onErrorHandler([this](Modbus::Error mbError, uint32_t token)
						{
							loge("Modbus RTU Error!!!");
							loge("Modbus RTU (Token: %d) Error response: %02X - %s", token, (int)mbError, (const char *)ModbusError(mbError));
							return true;
						});
					}
					#endif
				}
				else
				{
					_MBRTUserver.begin(Serial2);
					_MBRTUserver.useModbusRTU();
					logd("Modbus RTU started");					
				}

			}
			logd("Before xTimerStart _NetworkSelection: %d", _NetworkSelection);
			xTimerStart(mqttReconnectTimer, 0);
			setState(OnLine);
		}
	}

	void IOT::GoOffline()
	{
		logd("GoOffline");
		xTimerStop(mqttReconnectTimer, 0); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
		_webLog.end();
		_dnsServer.stop();
		logd("GoOffline RTU");
		if (_ModbusMode == RTU)
		{
			_MBRTUserver.end();
		}
		MDNS.end();
		if (_networkState == OnLine)
		{
			setState(OffLine);
		}
	}

	void IOT::setState(NetworkState newState)
	{
		NetworkState oldState = _networkState;
		_networkState = newState;
		logd("_networkState: %s", _networkState == Boot ? "Boot" : _networkState == ApState	 ? "ApState"
															   : _networkState == Connecting ? "Connecting"
															   : _networkState == OnLine	 ? "OnLine"
																							 : "OffLine");
		UpdateOledDisplay();
		switch (newState)
		{
		case OffLine:
			WiFi.disconnect(true);
			WiFi.mode(WIFI_OFF);
			DisconnectModem();
			DisconnectEthernet();
			break;
		case ApState:
			if ((oldState == Connecting) || (oldState == OnLine))
			{
				WiFi.disconnect(true);
				DisconnectModem();
				DisconnectEthernet();
			}
			WiFi.mode(WIFI_AP);
			if (WiFi.softAP(_AP_SSID, _AP_Password))
			{
				IPAddress IP = WiFi.softAPIP();
				logi("WiFi AP SSID: %s PW: %s", _AP_SSID.c_str(), _AP_Password.c_str());
				logd("AP IP address: %s", IP.toString().c_str());
				_dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
				_dnsServer.start(DNS_PORT, "*", IP);
			}
			_waitInAPTimeStamp = millis();
			break;
		case Connecting:
			_NetworkConnectionStart = millis();
			if (_NetworkSelection == WiFiMode)
			{
				logd("WiFiMode, trying to connect to %s", _SSID.c_str());
				WiFi.setHostname(_AP_SSID.c_str());
				WiFi.mode(WIFI_STA);
				WiFi.begin(_SSID, _WiFi_Password);
			}
			else if (_NetworkSelection == EthernetMode)
			{
				if (ConnectEthernet() == ESP_OK)
				{
					logd("Ethernet succeeded");
				}
				else
				{
					loge("Failed to connect to Ethernet");
				}
			}
			else if (_NetworkSelection == ModemMode)
			{
				if (ConnectModem() == ESP_OK)
				{
					logd("Modem succeeded");
				}
				else
				{
					loge("Failed to connect to 4G Modem");
				}
			}
			break;
		case OnLine:
			logd("State: Online");
			break;
		default:
			break;
		}
	}

	void IOT::HandleIPEvent(int32_t event_id, void *event_data)
	{
		ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
		if (event_id == IP_EVENT_PPP_GOT_IP || event_id == IP_EVENT_ETH_GOT_IP)
		{
			const esp_netif_ip_info_t *ip_info = &event->ip_info;
			logi("Got IP Address");
			logi("~~~~~~~~~~~");
			logi("IP:" IPSTR, IP2STR(&ip_info->ip));
			sprintf(_Current_IP, IPSTR, IP2STR(&ip_info->ip));
			logi("IPMASK:" IPSTR, IP2STR(&ip_info->netmask));
			logi("Gateway:" IPSTR, IP2STR(&ip_info->gw));
			logi("~~~~~~~~~~~");
			GoOnline();
		}
		else if (event_id == IP_EVENT_PPP_LOST_IP)
		{
			logi("Modem Disconnect from PPP Server");
			GoOffline();
		}
		else if (event_id == IP_EVENT_ETH_LOST_IP)
		{
			logi("Ethernet Disconnect");
			GoOffline();
		}
		else if (event_id == IP_EVENT_GOT_IP6)
		{
			ip_event_got_ip6_t *event = (ip_event_got_ip6_t *)event_data;
			logi("Got IPv6 address " IPV6STR, IPV62STR(event->ip6_info.ip));
		}
		else
		{
			logd("IP event! %d", (int)event_id);
		}
	}

	esp_err_t IOT::ConnectEthernet()
	{
		#ifdef HasEthernet
		esp_err_t ret = ESP_OK;
		logd("ConnectEthernet");
		if ((ret = gpio_install_isr_service(0)) != ESP_OK)
		{
			if (ret == ESP_ERR_INVALID_STATE)
			{
				logw("GPIO ISR handler has been already installed");
				ret = ESP_OK; // ISR handler has been already installed so no issues
			}
			else
			{
				logd("GPIO ISR handler install failed");
			}
		}
		spi_bus_config_t buscfg = {
			.mosi_io_num = ETH_MOSI,
			.miso_io_num = ETH_MISO,
			.sclk_io_num = ETH_SCK,
			.quadwp_io_num = -1,
			.quadhd_io_num = -1,
		};
		if ((ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO)) != ESP_OK)
		{
			logd("SPI host #1 init failed");
			return ret;
		}
		uint8_t base_mac_addr[6];
		if ((ret = esp_efuse_mac_get_default(base_mac_addr)) == ESP_OK)
		{
			uint8_t local_mac_1[6];
			esp_derive_local_mac(local_mac_1, base_mac_addr);
			logi("ETH MAC: %02X:%02X:%02X:%02X:%02X:%02X", local_mac_1[0], local_mac_1[1], local_mac_1[2], local_mac_1[3], local_mac_1[4], local_mac_1[5]);
			eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG(); // Init common MAC and PHY configs to default
			eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
			phy_config.phy_addr = 1;
			phy_config.reset_gpio_num = ETH_RST;
			spi_device_interface_config_t spi_devcfg = {
				.command_bits = 16, // Actually it's the address phase in W5500 SPI frame
				.address_bits = 8,	// Actually it's the control phase in W5500 SPI frame
				.mode = 0,
				.clock_speed_hz = 25 * 1000 * 1000,
				.spics_io_num = ETH_SS,
				.queue_size = 20,
			};
			spi_device_handle_t spi_handle;
			if ((ret = spi_bus_add_device(SPI2_HOST, &spi_devcfg, &spi_handle)) != ESP_OK)
			{
				loge("spi_bus_add_device failed");
				return ret;
			}
			eth_w5500_config_t w5500_config = ETH_W5500_DEFAULT_CONFIG(spi_handle);
			w5500_config.int_gpio_num = ETH_INT;
			esp_eth_mac_t *mac = esp_eth_mac_new_w5500(&w5500_config, &mac_config);
			esp_eth_phy_t *phy = esp_eth_phy_new_w5500(&phy_config);
			_eth_handle = NULL;
			esp_eth_config_t eth_config_spi = ETH_DEFAULT_CONFIG(mac, phy);
			if ((ret = esp_eth_driver_install(&eth_config_spi, &_eth_handle)) != ESP_OK)
			{
				loge("esp_eth_driver_install failed");
				return ret;
			}
			if ((ret = esp_eth_ioctl(_eth_handle, ETH_CMD_S_MAC_ADDR, local_mac_1)) != ESP_OK) // set mac address
			{
				logd("SPI Ethernet MAC address config failed");
			}
			esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH(); // Initialize the Ethernet interface
			_netif = esp_netif_new(&cfg);
			assert(_netif);
			if (!_useDHCP)
			{
				esp_netif_dhcpc_stop(_netif);
				esp_netif_ip_info_t ipInfo;
				IPAddress ip;
				ip.fromString(_Static_IP);
				ipInfo.ip.addr = static_cast<uint32_t>(ip);
				ip.fromString(_Subnet_Mask);
				ipInfo.netmask.addr = static_cast<uint32_t>(ip);
				ip.fromString(_Gateway_IP);
				ipInfo.gw.addr = static_cast<uint32_t>(ip);
				if ((ret = esp_netif_set_ip_info(_netif, &ipInfo)) != ESP_OK)
				{
					loge("esp_netif_set_ip_info failed: %d", ret);
					return ret;
				}
			}
			_eth_netif_glue = esp_eth_new_netif_glue(_eth_handle);
			if ((ret = esp_netif_attach(_netif, _eth_netif_glue)) != ESP_OK)
			{
				loge("esp_netif_attach failed");
				return ret;
			}
			if ((ret = esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &on_ip_event, this)) != ESP_OK)
			{
				loge("esp_event_handler_register IP_EVENT->IP_EVENT_ETH_GOT_IP failed");
				return ret;
			}
			if ((ret = esp_eth_start(_eth_handle)) != ESP_OK)
			{
				loge("esp_netif_attach failed");
				return ret;
			}
		}
		return ret;
		#else
		loge("Ethernet not supported on this device");
		return ESP_ERR_NOT_SUPPORTED;
		#endif
	}

	void IOT::wakeup_modem(void)
	{
		#ifdef HasLTE
		pinMode(LTE_PWR_EN, OUTPUT);
		#ifdef LTE_AIRPLANE_MODE 
		pinMode(LTE_AIRPLANE_MODE, OUTPUT); // turn off airplane mode
		digitalWrite(LTE_AIRPLANE_MODE, HIGH);
		#endif
		digitalWrite(LTE_PWR_EN, LOW);
		delay(1000);
		logd("Power on the modem");
		digitalWrite(LTE_PWR_EN, HIGH);
		delay(2000);
		logd("Modem is powered up and ready");
		#endif
	}

	esp_err_t IOT::ConnectModem()
	{
		#ifdef HasLTE
		logd("ConnectModem");
		esp_err_t ret = ESP_OK;
		wakeup_modem();
		esp_netif_config_t ppp_netif_config = ESP_NETIF_DEFAULT_PPP(); // Initialize lwip network interface in PPP mode
		_netif = esp_netif_new(&ppp_netif_config);
		assert(_netif);
		ESP_ERROR_CHECK(modem_init_network(_netif, _APN.c_str(), _SIM_PIN.c_str())); // Initialize the PPP network and register for IP event
		if (_SIM_Username.length() > 0)
		{
			esp_netif_ppp_set_auth(_netif, NETIF_PPP_AUTHTYPE_PAP, _SIM_Username.c_str(), _SIM_Password.c_str());
		}
		ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, on_ip_event, this));
		int retryCount = 3;
		while (retryCount-- != 0)
		{
			if (!modem_check_sync())
			{
				logw("Modem does not respond, maybe in DATA mode? ...exiting network mode");
				modem_stop_network();
				if (!modem_check_sync())
				{
					logw("Modem does not respond to AT ...restarting");
					modem_reset();
					logi("Restarted");
				}
				continue;
			}
			if (!modem_check_signal())
			{
				logw("Poor signal ...will check after 5s");
				vTaskDelay(pdMS_TO_TICKS(5000));
				continue;
			}
			if (!modem_start_network())
			{
				loge("Modem could not enter network mode ...will retry after 10s");
				vTaskDelay(pdMS_TO_TICKS(10000));
				continue;
			}
			break;
		}
		logi("Modem has acquired network");
		return ret;
		#else
		loge("LTE not supported on this device");
		return ESP_ERR_NOT_SUPPORTED;
		#endif
	}

	void IOT::DisconnectModem()
	{
		#ifdef HasLTE
		if (digitalRead(LTE_PWR_EN) == HIGH)
		{
			#ifdef LTE_AIRPLANE_MODE
			digitalWrite(LTE_AIRPLANE_MODE, LOW); // turn on airplane mode
			#endif
			digitalWrite(LTE_PWR_EN, LOW); // turn off power to the modem
			modem_stop_network();
			modem_deinit_network();
			if (_netif != NULL)
			{
				esp_netif_destroy(_netif);
				_netif = NULL;
			}
			ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, ESP_EVENT_ANY_ID, on_ip_event));
		}
		#endif
	}

	void IOT::DisconnectEthernet()
	{
		#ifdef HasEthernet
		if (_eth_handle != NULL)
		{
			ESP_ERROR_CHECK(esp_eth_stop(_eth_handle));
			_eth_handle = NULL;
			if (_eth_netif_glue != NULL)
			{
				ESP_ERROR_CHECK(esp_eth_del_netif_glue(_eth_netif_glue));
				_eth_netif_glue = NULL;
			}
			if (_netif != NULL)
			{
				esp_netif_destroy(_netif);
				_netif = NULL;
			}
			ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, ESP_EVENT_ANY_ID, on_ip_event));
		}
		#endif
	}

// #pragma endregion Network

// #pragma region Modbus

	void IOT::registerMBTCPWorkers(FunctionCode fc, MBSworker worker)
	{
		if (_ModbusMode == TCP)
		{
			_MBserver.registerWorker(_modbusID, fc, worker);
		}
		else
		{
			_MBRTUserver.registerWorker(_modbusID, fc, worker);					
		}
	}
	
	boolean IOT::ModbusBridgeEnabled()
	{
		return _useModbusBridge && (_ModbusMode == TCP);
	}

	Modbus::Error IOT::SendToModbusBridgeAsync(ModbusMessage& request)
	{
		Modbus::Error mbError = INVALID_SERVER;
		#ifdef HasRS485
		if (ModbusBridgeEnabled())
		{
			logv("SendToModbusBridge Token=%08X", Token);
			if (_MBclientRTU.pendingRequests() < MODBUS_RTU_REQUEST_QUEUE_SIZE)
			{
				mbError = _MBclientRTU.addRequest(request, nextToken());
				uint32_t nextToken();
				mbError = SUCCESS;
			}
			else
			{
				mbError = REQUEST_QUEUE_FULL;
			}
		}
		#endif
		return mbError;
	}

	ModbusMessage IOT::SendToModbusBridgeSync(ModbusMessage request)
	{
		#ifdef HasRS485
		if (ModbusBridgeEnabled())
		{
			uint32_t token = nextToken();
			logd("ForwardToModbusBridge token: %08X", token);
			return _MBclientRTU.syncRequest(request, token);
		}
		#endif
		ModbusMessage response;
		response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
		return response;
	}

	uint16_t IOT::getMBBaseAddress(IOTypes type)
	{
		switch(type)
		{
			case DigitalInputs:
				return _discrete_input_base_addr;
				break;
			case DigitalOutputs:
				return _coil_base_addr;
				break;
				
			case AnalogInputs:
				return _input_register_base_addr;
				break;
				
			case AnalogOutputs:
				return _holding_register_base_addr;
				break;
		}	
		return 0;
	}

// #pragma endregion Modbus

// #pragma region MQTT

	void IOT::HandleMQTT(int32_t event_id, void *event_data)
	{
		auto event = (esp_mqtt_event_handle_t)event_data;
		esp_mqtt_client_handle_t client = event->client;
		JsonDocument doc;
		switch ((esp_mqtt_event_id_t)event_id)
		{
		case MQTT_EVENT_CONNECTED:
			logi("Connected to MQTT.");
			char buf[128];
			sprintf(buf, "%s/set/#", _rootTopicPrefix);
			esp_mqtt_client_subscribe(client, buf, 0);
			_iotCB->onMqttConnect();
			esp_mqtt_client_publish(client, _willTopic, "Offline", 0, 1, 0);
			break;
		case MQTT_EVENT_DISCONNECTED:
			logw("Disconnected from MQTT");
			if (_networkState == OnLine)
			{
				xTimerStart(mqttReconnectTimer, 5000);
			}
			break;

		case MQTT_EVENT_SUBSCRIBED:
			logi("MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
			break;
		case MQTT_EVENT_UNSUBSCRIBED:
			logi("MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
			break;
		case MQTT_EVENT_PUBLISHED:
			logi("MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
			break;
		case MQTT_EVENT_DATA:
			char topicBuf[256];
			snprintf(topicBuf, sizeof(topicBuf), "%.*s", event->topic_len, event->topic);
			char payloadBuf[256];
			snprintf(payloadBuf, sizeof(payloadBuf), "%.*s", event->data_len, event->data);
			logd("MQTT Message arrived [%s] %s", topicBuf, payloadBuf);
			_iotCB->onMqttMessage(topicBuf, payloadBuf);
			// if (deserializeJson(doc, event->data)) // not json!
			// {
			// 	logd("MQTT payload {%s} is not valid JSON!", event->data);
			// }
			// else
			// {
			// 	if (doc.containsKey("status"))
			// 	{
			// 		doc.clear();
			// 		doc["sw_version"] = APP_VERSION;
			// 		// doc["IP"] = WiFi.localIP().toString().c_str();
			// 		// doc["SSID"] = WiFi.SSID();
			// 		doc["uptime"] = formatDuration(millis() - _lastBootTimeStamp);
			// 		Publish("status", doc, true);
			// 	}
			// 	else
			// 	{
			// 		_iotCB->onMqttMessage(topicBuf, doc);
			// 	}
			// }
			break;
		case MQTT_EVENT_ERROR:
			loge("MQTT_EVENT_ERROR");
			if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
			{
				logi("Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
			}
			break;
		default:
			logi("Other event id:%d", event->event_id);
			break;
		}
	}

	void IOT::ConnectToMQTTServer()
	{
		if (_networkState == OnLine)
		{
			if (_useMQTT && _mqttServer.length() > 0) // mqtt configured?
			{
				logd("Connecting to MQTT: %s:%d", _mqttServer.c_str(), _mqttPort);
				int len = strlen(_AP_SSID.c_str());
				strncpy(_rootTopicPrefix, _AP_SSID.c_str(), len);
				logd("rootTopicPrefix: %s", _rootTopicPrefix);
				sprintf(_willTopic, "%s/tele/LWT", _rootTopicPrefix);
				logd("_willTopic: %s", _willTopic);
				esp_mqtt_client_config_t mqtt_cfg = {};
				mqtt_cfg.host = _mqttServer.c_str();
				mqtt_cfg.port = _mqttPort;
				mqtt_cfg.username = _mqttUserName.c_str();
				mqtt_cfg.password = _mqttUserPassword.c_str();
				mqtt_cfg.client_id = _AP_SSID.c_str();
				mqtt_cfg.lwt_topic = _willTopic;
				mqtt_cfg.lwt_retain = 1;
				mqtt_cfg.lwt_msg = "Offline";
				mqtt_cfg.lwt_msg_len = 7;
				mqtt_cfg.transport = MQTT_TRANSPORT_OVER_TCP;
				// mqtt_cfg.cert_pem = (const char *)hivemq_ca_pem_start,
				// mqtt_cfg.skip_cert_common_name_check = true; // allow self-signed certs
				_mqtt_client_handle = esp_mqtt_client_init(&mqtt_cfg);
				esp_mqtt_client_register_event(_mqtt_client_handle, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, mqtt_event_handler, this);
				esp_mqtt_client_start(_mqtt_client_handle);
			}
		}
	}

	boolean IOT::Publish(const char *subtopic, JsonDocument &payload, boolean retained)
	{
		String s;
		serializeJson(payload, s);
		return Publish(subtopic, s.c_str(), retained);
	}

	boolean IOT::Publish(const char *subtopic, const char *value, boolean retained)
	{
		boolean rVal = false;
		if (_mqtt_client_handle != 0)
		{
			char buf[128];
			sprintf(buf, "%s/stat/%s", _rootTopicPrefix, subtopic);
			rVal = (esp_mqtt_client_publish(_mqtt_client_handle, buf, value, strlen(value), 1, retained) != -1);
			if (!rVal)
			{
				loge("**** Failed to publish MQTT message");
			}
		}
		return rVal;
	}

	boolean IOT::Publish(const char *topic, float value, boolean retained)
	{
		char buf[256];
		snprintf_P(buf, sizeof(buf), "%.1f", value);
		return Publish(topic, buf, retained);
	}

	boolean IOT::PublishMessage(const char *topic, JsonDocument &payload, boolean retained)
	{
		boolean rVal = false;
		if (_mqtt_client_handle != 0)
		{
			String s;
			serializeJson(payload, s);
			rVal = (esp_mqtt_client_publish(_mqtt_client_handle, topic, s.c_str(), s.length(), 0, retained) != -1);
			if (!rVal)
			{
				loge("**** Configuration payload exceeds MAX MQTT Packet Size, %d [%s] topic: %s", s.length(), s.c_str(), topic);
			}
		}
		return rVal;
	}

	boolean IOT::PublishHADiscovery(JsonDocument &payload)
	{
		boolean rVal = false;
		if (_mqtt_client_handle != 0)
		{
			char topic[64];
			sprintf(topic, "%s/device/%s_%X/config", HOME_ASSISTANT_PREFIX, TAG, getUniqueId());
			rVal = PublishMessage(topic, payload, true);
		}
		return rVal;
	}

	std::string IOT::getRootTopicPrefix()
	{
		std::string s(_rootTopicPrefix);
		return s;
	};

	std::string IOT::getThingName()
	{
		std::string s(_AP_SSID.c_str());
		return s;
	}

	void IOT::PublishOnline()
	{
		if (!_publishedOnline)
		{
			if (_mqtt_client_handle != 0)
			{
				if (!_publishedOnline)
				{
					if (esp_mqtt_client_publish(_mqtt_client_handle, _willTopic, "Online", 0, 1, 1) != -1)
					{
						_publishedOnline = true;
					}
				}
			}
		}
	}

// #pragma endregion MQTT

} // namespace CLASSICDIY