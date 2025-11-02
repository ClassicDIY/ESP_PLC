#include <Arduino.h>
#include "Log.h"
#include "IOT.h"
#include "PLC.h"
#include "style.html"
#include "PLC.html"

namespace CLASSICDIY
{
	static AsyncWebServer _asyncServer(ASYNC_WEBSERVER_PORT);
	static AsyncWebSocket _webSocket("/ws_home");
	IOT _iot = IOT();

// #region Setup

	void PLC::addApplicationConfigs(String &page)
	{
		#if AI_PINS > 0
		String appFields = app_config_fields;
		String appConvs;
		String scriptConvs;
		for (int i = 0; i < AI_PINS; i++)
		{
			String conv_flds(analog_conv_flds);
			String conv_script(app_validateInputs);
			conv_flds.replace("{An}", "A" + String(i));
			conv_script.replace("{An}", "A" + String(i));
			conv_flds.replace("{minV}", String(_AnalogSensors[i].minV(), 1));
			conv_flds.replace("{minT}", String(_AnalogSensors[i].minT(), 1));
			conv_flds.replace("{maxV}", String(_AnalogSensors[i].maxV(), 1));
			conv_flds.replace("{maxT}", String(_AnalogSensors[i].maxT(), 1));
			scriptConvs += conv_script;
			appConvs += conv_flds;	
		}
		appFields.replace("{aconv}", appConvs);
		page += appFields;
		page.replace("{validateInputs}", scriptConvs);
		#endif
		// Bridge app settings
		String appBridgeFields = app_modbusBridge;
		appBridgeFields.replace("{inputBridgeID}",  String(_inputID));
		appBridgeFields.replace("{inputRegBridge}",  String(_inputAddress));
		appBridgeFields.replace("{inputRegBridgeCount}",  String(_inputCount));
		appBridgeFields.replace("{coilBridgeID}",  String(_coilID));
		appBridgeFields.replace("{coilBridge}",  String(_coilAddress));
		appBridgeFields.replace("{coilBridgeCount}",  String(_coilCount));
		appBridgeFields.replace("{discreteBridgeID}",  String(_discreteID));
		appBridgeFields.replace("{discreteBridge}",  String(_discreteAddress));
		appBridgeFields.replace("{discreteBridgeCount}",  String(_discreteCount));
		appBridgeFields.replace("{holdingBridgeID}",  String(_holdingID));
		appBridgeFields.replace("{holdingRegBridge}",  String(_holdingAddress));
		appBridgeFields.replace("{holdingRegBridgeCount}",  String(_holdingCount));
		page.replace("{modbusBridgeAppSettings}", appBridgeFields);
	}

	void PLC::onSubmitForm(AsyncWebServerRequest *request)
	{
		for (int i = 0; i < AI_PINS; i++)
		{
			String ain = "A" + String(i);
			if (request->hasParam(ain + "_min", true))
			{
				_AnalogSensors[i].SetMinV(request->getParam(ain + "_min", true)->value().toFloat());
			}
			if (request->hasParam(ain + "_min_t", true))
			{
				_AnalogSensors[i].SetMinT(request->getParam(ain + "_min_t", true)->value().toFloat());
			}	
			if (request->hasParam(ain + "_max", true))
			{
				_AnalogSensors[i].SetMaxV(request->getParam(ain + "_max", true)->value().toFloat());
			}
			if (request->hasParam(ain + "_max_t", true))
			{
				_AnalogSensors[i].SetMaxT(request->getParam(ain + "_max_t", true)->value().toFloat());
			}		
		}
		// Bridge app settings
		if (request->hasParam("inputBridgeID", true)) { _inputID = request->getParam("inputBridgeID", true)->value().toInt(); }
		if (request->hasParam("inputRegBridge", true)) { _inputAddress = request->getParam("inputRegBridge", true)->value().toInt(); }
		if (request->hasParam("inputRegBridgeCount", true)) { _inputCount = request->getParam("inputRegBridgeCount", true)->value().toInt(); }
		if (request->hasParam("coilBridgeID", true)) { _coilID = request->getParam("coilBridgeID", true)->value().toInt(); }
		if (request->hasParam("coilBridge", true)) { _coilAddress = request->getParam("coilBridge", true)->value().toInt(); }
		if (request->hasParam("coilBridgeCount", true)) { _coilCount = request->getParam("coilBridgeCount", true)->value().toInt(); }
		if (request->hasParam("discreteBridgeID", true)) { _discreteID = request->getParam("discreteBridgeID", true)->value().toInt(); }
		if (request->hasParam("discreteBridge", true)) { _discreteAddress = request->getParam("discreteBridge", true)->value().toInt(); }
		if (request->hasParam("discreteBridgeCount", true)) { _discreteCount = request->getParam("discreteBridgeCount", true)->value().toInt(); }
		if (request->hasParam("holdingBridgeID", true)) { _holdingID = request->getParam("holdingBridgeID", true)->value().toInt(); }
		if (request->hasParam("holdingRegBridge", true)) { _holdingAddress = request->getParam("holdingRegBridge", true)->value().toInt(); }
		if (request->hasParam("holdingRegBridgeCount", true)) { _holdingCount = request->getParam("holdingRegBridgeCount", true)->value().toInt(); }
	}

	void PLC::onSaveSetting(JsonDocument &doc)
	{
		JsonObject plc = doc["plc"].to<JsonObject>();
		for (int i = 0; i < AI_PINS; i++)
		{
			String ain = "A" + String(i);
			plc[ain + "_minV"] = _AnalogSensors[i].minV();
			plc[ain + "_minT"] = _AnalogSensors[i].minT();
			plc[ain + "_maxV"] = _AnalogSensors[i].maxV();
			plc[ain + "_maxT"] = _AnalogSensors[i].maxT();
		}
		// Bridge app settings
		plc["inputBridgeID"] = _inputID;
		plc["inputRegBridge"] = _inputAddress;
		plc["inputRegBridgeCount"] = _inputCount;
		plc["coilBridgeID"] = _coilID;
		plc["coilBridge"] = _coilAddress;
		plc["coilBridgeCount"] = _coilCount;
		plc["discreteBridgeID"] = _discreteID;
		plc["discreteBridge"] = _discreteAddress;
		plc["discreteBridgeCount"] = _discreteCount;
		plc["holdingBridgeID"] = _holdingID;
		plc["holdingRegBridge"] = _holdingAddress;
		plc["holdingRegBridgeCount"] = _holdingCount;
	}

	void PLC::onLoadSetting(JsonDocument &doc)
	{
		JsonObject plc = doc["plc"].as<JsonObject>();
		for (int i = 0; i < AI_PINS; i++)
		{
			String ain = "A" + String(i);
			plc[ain + "_minV"].isNull() ? _AnalogSensors[i].SetMinV(1.0) : _AnalogSensors[i].SetMinV(plc[ain + "_minV"].as<float>());
			plc[ain + "_minT"].isNull() ? _AnalogSensors[i].SetMinT(0.0) : _AnalogSensors[i].SetMinT(plc[ain + "_minT"].as<float>());
			plc[ain + "_maxV"].isNull() ? _AnalogSensors[i].SetMaxV(5.0) : _AnalogSensors[i].SetMaxV(plc[ain + "_maxV"].as<float>());
			plc[ain + "_maxT"].isNull() ? _AnalogSensors[i].SetMaxT(100.0) : _AnalogSensors[i].SetMaxT(plc[ain + "_maxT"].as<float>());
		}
		_inputID = plc["inputBridgeID"].isNull() ? 0 : plc["inputBridgeID"].as<uint8_t>();
		_inputAddress = plc["inputRegBridge"].isNull() ? 0 : plc["inputRegBridge"].as<uint16_t>();
		_inputCount = plc["inputRegBridgeCount"].isNull() ? 0 : plc["inputRegBridgeCount"].as<uint8_t>();
		_coilID = plc["coilBridgeID"].isNull() ? 0 : plc["coilBridgeID"].as<uint8_t>();
		_coilAddress = plc["coilBridge"].isNull() ? 0 : plc["coilBridge"].as<uint16_t>();
		_coilCount = plc["coilBridgeCount"].isNull() ? 0 : plc["coilBridgeCount"].as<uint8_t>();
		_discreteID = plc["discreteBridgeID"].isNull() ? 0 : plc["discreteBridgeID"].as<uint8_t>();
		_discreteAddress = plc["discreteBridge"].isNull() ? 0 : plc["discreteBridge"].as<uint16_t>();
		_discreteCount = plc["discreteBridgeCount"].isNull() ? 0 : plc["discreteBridgeCount"].as<uint8_t>();
		_holdingID = plc["holdingBridgeID"].isNull() ? 0 : plc["holdingBridgeID"].as<uint8_t>();
		_holdingAddress = plc["holdingRegBridge"].isNull() ? 0 : plc["holdingRegBridge"].as<uint16_t>();
		_holdingCount = plc["holdingRegBridgeCount"].isNull() ? 0 : plc["holdingRegBridgeCount"].as<uint8_t>();
	}

	void PLC::setup()
	{
		logd("setup");
		_iot.Init(this, &_asyncServer);
		uint16_t coilCount = _iot.ModbusBridgeEnabled() ? _coilCount + DO_PINS : DO_PINS;
		_digitalOutputCoils = CoilData(coilCount);
		uint16_t discreteCount = _iot.ModbusBridgeEnabled() ? _discreteCount : 0;
		discreteCount += DI_PINS;
		_digitalInputDiscretes = CoilData(discreteCount, false);
		_analogInputCount = _iot.ModbusBridgeEnabled() ? _inputCount + AI_PINS : AI_PINS;
		_analogOutputCount = _iot.ModbusBridgeEnabled() ? _holdingCount + AO_PINS : AO_PINS;
		_asyncServer.on("/", HTTP_GET, [this](AsyncWebServerRequest *request)
		{
			String page = home_html;
			page.replace("{style}", style);
			page.replace("{n}", _iot.getThingName().c_str());
			page.replace("{v}", APP_VERSION);
			char desc_buf[64];
			sprintf(desc_buf, "Modbus Discretes: %d-%d", _iot.getMBBaseAddress(IOTypes::DigitalInputs), _iot.getMBBaseAddress(IOTypes::DigitalInputs) + _digitalInputDiscretes.coils());
			page.replace("{digitalInputDesc}", desc_buf);
			std::string s;
			for (int i = 0; i < _digitalInputDiscretes.coils(); i++)
			{
				s += "<div class='box' id=DI";
				s += std::to_string(i);
				s += "> DI";
				s += std::to_string(i);
				s += "</div>";
			}
			page.replace("{digitalInputs}", s.c_str());
			s.clear();
			sprintf(desc_buf, "Modbus Coils: %d-%d", _iot.getMBBaseAddress(IOTypes::DigitalOutputs), _iot.getMBBaseAddress(IOTypes::DigitalOutputs) + _digitalOutputCoils.coils());
			page.replace("{digitalOutputDesc}", desc_buf);
			for (int i = 0; i < _digitalOutputCoils.coils(); i++)
			{
				s += "<div class='box' id=DO";
				s += std::to_string(i);
				s += "> DO";
				s += std::to_string(i);
				s += "</div>";
			}
			page.replace("{digitalOutputs}", s.c_str());
			s.clear();
			sprintf(desc_buf, "Modbus Input Registers: %d-%d", _iot.getMBBaseAddress(IOTypes::AnalogInputs), _iot.getMBBaseAddress(IOTypes::AnalogInputs) + _analogInputCount);
			page.replace("{analogInputDesc}", desc_buf);
			for (int i = 0; i < _analogInputCount; i++)
			{
				s += "<div class='box' id=AI";
				s += std::to_string(i);
				s += "> AI";
				s += std::to_string(i);
				s += "</div>";
			}
			page.replace("{analogInputs}", s.c_str());
			s.clear();
			sprintf(desc_buf, "Modbus Holding Registers: %d-%d", _iot.getMBBaseAddress(IOTypes::AnalogOutputs), _iot.getMBBaseAddress(IOTypes::AnalogOutputs) + _analogOutputCount);
			page.replace("{analogOutputDesc}", desc_buf);
			for (int i = 0; i < _analogOutputCount; i++)
			{
				s += "<div class='box' id=AO";
				s += std::to_string(i);
				s += "> AO";
				s += std::to_string(i);
				s += "</div>";
			}
			page.replace("{analogOutputs}", s.c_str());
			request->send(200, "text/html", page); 
		});
		_asyncServer.addHandler(&_webSocket).addMiddleware([this](AsyncWebServerRequest *request, ArMiddlewareNext next)
		{
			// ws.count() is the current count of WS clients: this one is trying to upgrade its HTTP connection
			if (_webSocket.count() > 1) 
			{
				// if we have 2 clients or more, prevent the next one to connect
				request->send(503, "text/plain", "Server is busy");
			} else 
			{
				// process next middleware and at the end the handler
				next();
			}
		});
		_webSocket.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
		{
			(void)len;
			if (type == WS_EVT_CONNECT) {
				_lastMessagePublished.clear(); //force a broadcast
				client->setCloseClientOnQueueFull(false);
				client->ping();
			} else if (type == WS_EVT_DISCONNECT) {
				// logi("Home Page Disconnected!");
			} else if (type == WS_EVT_ERROR) {
				loge("ws error");
			// } else if (type == WS_EVT_PONG) {
            // 	logd("ws pong");
        	} 
		});
	}

	void PLC::CleanUp()
	{
		_webSocket.cleanupClients(); // cleanup disconnected clients or too many clients
	}

	void PLC::Monitor()
	{
		for (int i = 0; i < AI_PINS; i++)
		{
			_AnalogSensors[i].Run();
		}
		if (_iot.getNetworkState() == OnLine && _iot.ModbusBridgeEnabled())
		{
			ModbusMessage forward;
			if (_discreteCount > 0)
			{
				uint8_t err = forward.setMessage(_discreteID, READ_DISCR_INPUT, _discreteAddress, _discreteCount);
				if (err == SUCCESS)
				{
					Modbus::Error error = _iot.SendToModbusBridgeAsync(forward);
					if (error != SUCCESS)
					{
						logd("Error forwarding FC02 to modbus bridge device Id:%d Error: 0X%x", _discreteID, error);
					}
				}
				else
				{
					loge("poll discrete error: 0X%x", err);
				}
			}
		}
	}

	void PLC::Process()
	{
		_iot.Run();
		if (_iot.getNetworkState() == OnLine)
		{
			JsonDocument doc;
			doc.clear();
			#if DI_PINS > 0
			for (int i = 0; i < DI_PINS; i++)
			{
				_digitalInputDiscretes.set(i, _DigitalSensors[i].Level());
			}
			#endif
			for (int i = 0; i < _digitalInputDiscretes.coils(); i++)
			{
				std::stringstream ss;
				ss << "DI" << i;
				doc[ss.str()] = _digitalInputDiscretes[i] ? "High" : "Low";
			}
			for (int i = 0; i < AI_PINS; i++)
			{
				std::stringstream ss;
				ss << "AI" << i;
				doc[ss.str()] = _AnalogSensors[i].Level();
			}
			#if DO_PINS > 0
			for (int i = 0; i < DO_PINS; i++)
			{
				_digitalOutputCoils.set(i, _Coils[i].Level());
			}
			#endif
			for (int i = 0; i < _digitalOutputCoils.coils(); i++)
			{
				std::stringstream ss;
				ss << "DO" << i;
				doc[ss.str()] = _digitalOutputCoils[i] ? "On" : "Off";
			}
			#if AO_PINS > 0
			for (int i = 0; i < AO_PINS; i++)
			{
				std::stringstream ss;
				ss << "AO" << i;
				doc[ss.str()] = _PWMOutputs[i].GetDutyCycle();
			}
			#endif
			String s;
			serializeJson(doc, s);
			DeserializationError err = deserializeJson(doc, s);
			if (err)
			{
				loge("deserializeJson() failed: %s", err.c_str());
			}
			if (_lastMessagePublished == s) // anything changed?
			{
				return;
			}
			_iot.PublishOnline();
			_iot.Publish("readings", s.c_str(), false);
			_lastMessagePublished = s;
			_webSocket.textAll(s);
			logv("Published readings: %s", s.c_str());
		}
	}

// #endregion

// #pragma region Modbus
	void PLC::onNetworkConnect()
	{
		// READ_INPUT_REGISTER
		auto modbusFC04 = [this](ModbusMessage request) -> ModbusMessage
		{
			ModbusMessage response;
			uint16_t addr = 0;
			uint16_t words = 0;
			request.get(2, addr);
			request.get(4, words);
			// logd("READ_INPUT_REGISTER %d %d[%d]", request.getFunctionCode(), addr, words);
			addr -= _iot.getMBBaseAddress(AnalogInputs);
			if ((addr + words) > AI_PINS)
			{
				logw("READ_INPUT_REGISTER error: %d", (addr + words));
				response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
			}
			else
			{
				#if AI_PINS > 0
				response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)(words * 2));
				for (int i = addr; i < (addr + words); i++)
				{
					response.add((uint16_t)_AnalogSensors[i].Level());
				}
				#endif
			}
			return response;
		};

		// READ_DISCR_INPUT
		auto modbusFC02 = [this](ModbusMessage request) -> ModbusMessage
		{
			ModbusMessage response; 
			uint16_t start = 0;
			uint16_t numregs = 0;
			request.get(2, start, numregs);
			logd("READ_DISCR_INPUT FC%d %d[%d]", request.getFunctionCode(), start, numregs);
			start -= _iot.getMBBaseAddress(DigitalInputs);
			if ((start + numregs) <= _digitalInputDiscretes.coils())
			{
				#if DI_PINS > 0
				for (int i = 0; i < DI_PINS; i++)
				{
					_digitalInputDiscretes.set(i, _DigitalSensors[i].Level());
				}
				#endif
				if ((start + numregs) > DI_PINS) // query bridge device?
				{
					uint16_t index = start >= DI_PINS ? start - DI_PINS : 0; // number and address of bridge discretes
					uint16_t count = start >= DI_PINS ? numregs : (start + numregs) - DI_PINS;
					logd("Bridge Discrete: %d[%d]", index, count);
					ModbusMessage forward;
					uint8_t err = forward.setMessage(_discreteID, request.getFunctionCode(), index, count);
					if (err != SUCCESS)
					{
						loge("ModbusMessage Read discrete error: 0X%x", err);
						response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_VALUE);
					}
					else
					{
						ModbusMessage forwardedresponse = _iot.SendToModbusBridgeSync(forward);
						if (forwardedresponse.getError() != SUCCESS)
						{
							ModbusError e(forwardedresponse.getError());
							logd("Error forwarding FC%d to modbus bridge device Id:%d Error:  %02X - %s", forwardedresponse.getFunctionCode(), _coilID, (int)e, (const char *)e);
							response.setError(request.getServerID(), request.getFunctionCode(), (Error)e);
						}
						else
						{
							_digitalInputDiscretes.set(index + DI_PINS, count, (uint8_t *)forwardedresponse.data() + 3);
						}
					}
				}			
				vector<uint8_t> discreteSet = _digitalInputDiscretes.slice(start, numregs);
				response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)discreteSet.size(), discreteSet);
			}
			else
			{
				logw("READ_DISCR_INPUT error: %d", (start + numregs));
				response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
			}
			return response;
		};

		// READ_COIL
		auto modbusFC01 = [this](ModbusMessage request) -> ModbusMessage
		{
			ModbusMessage response; // The Modbus message we are going to give back
			uint16_t start = 0;
			uint16_t numCoils = 0;
			request.get(2, start, numCoils);
			logd("READ_COIL %d %d[%d]", request.getFunctionCode(), start, numCoils);
			// Address overflow?
			start -= _iot.getMBBaseAddress(DigitalOutputs);
			if ((start + numCoils) <= _digitalOutputCoils.coils())
			{
				#if DO_PINS > 0
				for (int i = 0; i < DO_PINS; i++)
				{
					_digitalOutputCoils.set(i, _Coils[i].Level());
				}
				#endif
				if ((start + numCoils) > DO_PINS) // query bridge device?
				{
					uint16_t index = start >= DO_PINS ? start - DO_PINS : 0; // number and address of bridge coils
					uint16_t count = start >= DO_PINS ? numCoils : (start + numCoils) - DO_PINS;
					logd("Bridge Coil: %d[%d]", index, count);
					ModbusMessage forward;
					uint8_t err = forward.setMessage(_coilID, request.getFunctionCode(), index, count);
					if (err != SUCCESS)
					{
						loge("ModbusMessage Read coil error: 0X%x", err);
						response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_VALUE);
					}
					else
					{
						ModbusMessage forwardedresponse = _iot.SendToModbusBridgeSync(forward);
						if (forwardedresponse.getError() != SUCCESS)
						{
							ModbusError e(forwardedresponse.getError());
							logd("Error forwarding FC%d to modbus bridge device Id:%d Error:  %02X - %s", forwardedresponse.getFunctionCode(), _coilID, (int)e, (const char *)e);
							response.setError(request.getServerID(), request.getFunctionCode(), (Error)e);
						}
						else
						{
							_digitalOutputCoils.set(index + DO_PINS, count, (uint8_t *)forwardedresponse.data() + 3);
						}
					}
				}
				vector<uint8_t> coilset = _digitalOutputCoils.slice(start, numCoils);
				response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)coilset.size(), coilset);
			}
			else
			{
				logw("READ_COIL error: %d", (start + numCoils));
				response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
			}
			return response;
		};

		// WRITE_COIL
		auto modbusFC05 = [this](ModbusMessage request) -> ModbusMessage
		{
			ModbusMessage response;
			// Request parameters are coil number and 0x0000 (OFF) or 0xFF00 (ON)
			uint16_t coilAddr = 0;
			uint16_t state = 0;
			request.get(2, coilAddr, state);
			logd("WRITE_COIL %d %d:%d", request.getFunctionCode(), coilAddr, state);
			uint16_t index = coilAddr - _iot.getMBBaseAddress(DigitalOutputs);
			// Is the coil number within the range of the coils?
			if (index < _digitalOutputCoils.coils())
			{

				// Looks like it. Is the ON/OFF parameter correct?
				if (state == 0x0000 || state == 0xFF00)
				{
					if (index >= DO_PINS)
					{
						coilAddr -= DO_PINS; // start of bridge coils
						ModbusMessage forward;
						uint8_t err = forward.setMessage(_coilID, request.getFunctionCode(), coilAddr, state);
						if (err != SUCCESS)
						{
							loge("ModbusMessage Write coil error: 0X%x", err);
							response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_VALUE);
						}
						else
						{
							ModbusMessage forwardedresponse = _iot.SendToModbusBridgeSync(forward);
							if (forwardedresponse.getError() != SUCCESS)
							{
								ModbusError e(forwardedresponse.getError());
								logd("Error forwarding FC%d to modbus bridge device Id:%d Error:  %02X - %s", forwardedresponse.getFunctionCode(), _coilID, (int)e, (const char *)e);
								response.setError(request.getServerID(), request.getFunctionCode(), (Error)e);
							}
							else
							{
								// All fine, return shortened echo response, like the standard says
								response = ECHO_RESPONSE; 
							}
						}
					}
					#if DO_PINS > 0
					// Set the native coil
					else if (_digitalOutputCoils.set(index, state))
					{
						_Coils[index].Set(state == 0xFF00 ? HIGH : LOW);
						response = ECHO_RESPONSE;
					}
					#endif
					else
					{
						response.setError(request.getServerID(), request.getFunctionCode(), SERVER_DEVICE_FAILURE);
					}
				}
				else
				{
					// Wrong data parameter
					response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_VALUE);
				}
			}
			return response;
		};

		// WRITE_MULT_COILS
		auto modbusFC0F = [this](ModbusMessage request) -> ModbusMessage
		{
			ModbusMessage response;
			// Request parameters are first coil to be set, number of coils, number of bytes and packed coil bytes
			uint16_t start = 0;
			uint16_t numCoils = 0;
			uint8_t numBytes = 0;
			uint16_t offset = 2; // Parameters start after serverID and FC
			offset = request.get(offset, start, numCoils, numBytes);
			logd("WRITE_MULT_COILS %d %d[%d]", request.getFunctionCode(), start, numCoils);
			start -= _iot.getMBBaseAddress(DigitalOutputs);
			if ((start + numCoils) <= _digitalOutputCoils.coils())
			{
				// Packed coils will fit in our storage
				if (numBytes == ((numCoils - 1) >> 3) + 1)
				{
					// Byte count seems okay, so get the packed coil bytes now
					vector<uint8_t> coilset;
					request.get(offset, coilset, numBytes);
					logd("offset: %d coilset: %d numCoils: %d numBytes: %d start: %d", offset, coilset.size(), numCoils, numBytes, start);
					// Now set the coils
					if (_digitalOutputCoils.set(start, numCoils, coilset))
					{
						#if DO_PINS > 0 
						// set native DO pins
						for (int i = 0; i < DO_PINS; i++)
						{
							_Coils[i].Set(_digitalOutputCoils[i]);
						}
						#endif
						uint16_t count = start >= DO_PINS ? numCoils : (start + numCoils) - DO_PINS;
						if (count > 0) //any coils to forward to RS485 device?
						{
							uint16_t index = start >= DO_PINS ? start : DO_PINS; // number and address of bridge coils
							logd("count: %d index: %d", count, index);
							CoilData bridgeCoilset = _digitalOutputCoils.slice(index, count); // remaining coils are forwarded to the modbus bridge
							ModbusMessage forward;
							uint16_t p1 = _coilAddress + index - DO_PINS;
							uint8_t err = forward.setMessage(_coilID, request.getFunctionCode(), p1, bridgeCoilset.coils(), bridgeCoilset.size(), bridgeCoilset.data());
							if (err != SUCCESS)
							{
								loge("ModbusMessage Write multiple coils error: 0X%x", err);
								response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_VALUE);
							}
							else
							{
								ModbusMessage forwardedresponse = _iot.SendToModbusBridgeSync(forward);
								if (forwardedresponse.getError() != SUCCESS)
								{
									ModbusError e(forwardedresponse.getError());
									logd("Error forwarding FC%d to modbus bridge device Id:%d Error:  %02X - %s", forwardedresponse.getFunctionCode(), _coilID, (int)e, (const char *)e);
									response.setError(request.getServerID(), request.getFunctionCode(), (Error)e);
								}
								else
								{
									response = ECHO_RESPONSE; // All fine, return shortened echo response, like the standard says
								}
							}
						}
						else
						{
							response = ECHO_RESPONSE; // All fine, return shortened echo response, like the standard says
						}
					}
					else
					{
						logd("SERVER_DEVICE_FAILURE Setting the coils seems to have failed");
						response.setError(request.getServerID(), request.getFunctionCode(), SERVER_DEVICE_FAILURE);
					}
				}
				else
				{
					logd("ILLEGAL_DATA_VALUE numBytes had a wrong value");
					response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_VALUE);
				}
			}
			else
			{
				logd("ILLEGAL_DATA_VALUE The given set will not fit to our coil storage");
				response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
			}
			return response;
		};

		// READ_HOLD_REGISTER
		auto modbusFC03 = [this](ModbusMessage request) -> ModbusMessage
		{
			ModbusMessage response; 
			uint16_t addr = 0;           // Start address
			uint16_t words = 0;          // # of words requested
			request.get(2, addr);        // read address from request
			request.get(4, words);       // read # of words from request
			logd("READ_HOLD_REGISTER FC%d %d[%d]", request.getFunctionCode(), addr, words);
			addr -= _iot.getMBBaseAddress(AnalogOutputs);
			if ((addr + words) <= _analogOutputCount)
			{
				response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)(words * 2));
				#if AO_PINS > 0
				for (int i = addr; i < words; i++)
				{
					response.add((uint16_t)_PWMOutputs[i].GetDutyCycle());
				}
				#endif
				if ((addr + words) > AO_PINS) // query bridge device?
				{
					uint16_t index = addr >= AO_PINS ? addr - AO_PINS : 0; // number and address of bridge discretes
					uint16_t count = addr >= AO_PINS ? words : (addr + words) - AO_PINS;
					logd("Bridge holding: %d[%d]", index, count);
					ModbusMessage forward;
					uint8_t err = forward.setMessage(_holdingID, request.getFunctionCode(), index, count);
					if (err != SUCCESS)
					{
						loge("ModbusMessage Read holding error: 0X%x", err);
						response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_VALUE);
					}
					else
					{
						ModbusMessage forwardedresponse = _iot.SendToModbusBridgeSync(forward);
						if (forwardedresponse.getError() != SUCCESS)
						{
							ModbusError e(forwardedresponse.getError());
							logd("Error forwarding FC%d to modbus bridge device Id:%d Error:  %02X - %s", forwardedresponse.getFunctionCode(), _coilID, (int)e, (const char *)e);
							response.setError(request.getServerID(), request.getFunctionCode(), (Error)e);
						}
						else
						{
							// _digitalInputDiscretes.set(index + DI_PINS, count, (uint8_t *)forwardedresponse.data() + 3);
						}
					}
				}			
				// vector<uint8_t> discreteSet = _digitalInputDiscretes.slice(start, numregs);
				// response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)discreteSet.size(), discreteSet);
			}
			else
			{
				logd("ILLEGAL_DATA_VALUE The given set will not fit to our storage");
				response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
			}
			return response;
		};

		// WRITE_HOLD_REGISTER
		auto modbusFC06 = [this](ModbusMessage request) -> ModbusMessage
		{
			ModbusMessage response; 
			uint16_t addr = 0; // register address
			uint16_t value = 0; // register value
			request.get(2, addr, value);
			logd("WRITE_HOLD_REGISTER FC%d %d[%d]", request.getFunctionCode(), addr, value);
			addr -= _iot.getMBBaseAddress(AnalogOutputs);
			if (addr < _analogOutputCount)
			{
				#if AO_PINS > 0
				_PWMOutputs[addr].SetDutyCycle(value);
				response = ECHO_RESPONSE;
				#endif
				if ((addr) > AO_PINS) // query bridge device?
				{
					uint16_t index = addr >= AO_PINS ? addr - AO_PINS : 0; // number and address of bridge discretes
					ModbusMessage forward;
					uint8_t err = forward.setMessage(_holdingID, request.getFunctionCode(), index, value);
					if (err != SUCCESS)
					{
						loge("ModbusMessage write holding error: 0X%x", err);
						response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_VALUE);
					}
					else
					{
						ModbusMessage forwardedresponse = _iot.SendToModbusBridgeSync(forward);
						if (forwardedresponse.getError() != SUCCESS)
						{
							ModbusError e(forwardedresponse.getError());
							logd("Error forwarding FC%d to modbus bridge device Id:%d Error:  %02X - %s", forwardedresponse.getFunctionCode(), _coilID, (int)e, (const char *)e);
							response.setError(request.getServerID(), request.getFunctionCode(), (Error)e);
						}
					}
				}			
			}
			else
			{
				logd("ILLEGAL_DATA_VALUE The given set will not fit to our storage");
				response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
			}
			return response;
		};

		// WRITE_MULT_REGISTERS
		auto modbusFC10 = [this](ModbusMessage request) -> ModbusMessage
		{
			ModbusMessage response; 
			uint16_t addr = 0;
			uint16_t numRegs = 0;
			uint8_t numBytes = 0;
			uint16_t offset = 2; // Parameters start after serverID and FC
			offset = request.get(offset, addr, numRegs, numBytes);
			logd("WRITE_MULT_REGISTERS %d %d[%d] (%d bytes)", request.getFunctionCode(), addr, numRegs, numBytes);
			addr -= _iot.getMBBaseAddress(AnalogOutputs);
			if (addr < _analogOutputCount)
			{
				if (numRegs == numBytes/2)
				{
					vector<uint8_t> regset;
					request.get(offset, regset, numBytes);
					logd("offset: %d coilset: %d numRegs: %d numBytes: %d start: %d", offset, regset.size(), numRegs, numBytes, addr);
					#if AO_PINS > 0
					// set native AO pins
					for (int i = addr; i < AO_PINS; i++)
					{
						int w = i * 2;
						logd("regset[%d]=%d %d", i, regset[i], regset[i+1]);
						if (regset[w] == 0)
						{
							_PWMOutputs[i].SetDutyCycle(regset[w+1]);
						}
					}
					response = ECHO_RESPONSE;
					#endif
				}
				uint16_t count = addr >= AO_PINS ? numRegs : (addr + numRegs) - AO_PINS;
				if (count > 0) //any registers to forward to RS485 device?
				{
					uint16_t index = addr >= AO_PINS ? addr - AO_PINS : 0; // number and address of bridge discretes
					ModbusMessage forward;
					// ToDo
					// uint8_t err = forward.setMessage(_holdingID, request.getFunctionCode(), index, value);
					// if (err != SUCCESS)
					// {
					// 	loge("ModbusMessage write holding error: 0X%x", err);
					// 	response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_VALUE);
					// }
					// else
					// {
					// 	ModbusMessage forwardedresponse = _iot.SendToModbusBridgeSync(forward);
					// 	if (forwardedresponse.getError() != SUCCESS)
					// 	{
					// 		ModbusError e(forwardedresponse.getError());
					// 		logd("Error forwarding FC%d to modbus bridge device Id:%d Error:  %02X - %s", forwardedresponse.getFunctionCode(), _coilID, (int)e, (const char *)e);
					// 		response.setError(request.getServerID(), request.getFunctionCode(), (Error)e);
					// 	}
					// }
				}			
			}
			else
			{
				logd("ILLEGAL_DATA_VALUE The given set will not fit to our storage");
				response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
			}
			return response;
		};

		_iot.registerMBTCPWorkers(READ_INPUT_REGISTER, modbusFC04);
		_iot.registerMBTCPWorkers(READ_DISCR_INPUT, modbusFC02);
		_iot.registerMBTCPWorkers(READ_COIL, modbusFC01);
		_iot.registerMBTCPWorkers(WRITE_COIL, modbusFC05);
		_iot.registerMBTCPWorkers(WRITE_MULT_COILS, modbusFC0F);
		_iot.registerMBTCPWorkers(READ_HOLD_REGISTER, modbusFC03);
		_iot.registerMBTCPWorkers(WRITE_HOLD_REGISTER, modbusFC06);
		_iot.registerMBTCPWorkers(WRITE_MULT_REGISTERS, modbusFC10);
	}
	
	bool PLC::onModbusMessage(ModbusMessage& msg)
	{
		bool rval = false;
		switch (msg.getFunctionCode())
		{
			case READ_DISCR_INPUT:
				rval = _digitalInputDiscretes.set(DI_PINS, _discreteCount, (uint8_t *)msg.data() + 3);
			break;
			case READ_HOLD_REGISTER:
				// rval = _digitalInputDiscretes.set(AO_PINS, _holdingCount, (uint8_t *)msg.data() + 3);
			break;
			case READ_COIL:
				rval = _digitalOutputCoils.set(DO_PINS, _coilCount, (uint8_t *)msg.data() + 3);
			break;
			case READ_INPUT_REGISTER:
				// rval = _digitalInputDiscretes.set(AI_PINS, _inputCount, (uint8_t *)msg.data() + 3);
			break;
		}
		return rval;
	}

// #pragma endregion Modbus

// #pragma region MQTT

	void PLC::onMqttConnect()
	{
		if (ReadyToPublish())
		{
			logd("Publishing discovery ");
			char buffer[STR_LEN];
			JsonDocument doc;
			JsonObject device = doc["device"].to<JsonObject>();
			device["sw_version"] = APP_VERSION;
			device["manufacturer"] = "ClassicDIY";
			sprintf(buffer, "%s (%X)",TAG, _iot.getUniqueId());
			device["model"] = buffer;

			JsonObject origin = doc["origin"].to<JsonObject>();
			origin["name"] = TAG;

			JsonArray identifiers = device["identifiers"].to<JsonArray>();
			sprintf(buffer, "%X", _iot.getUniqueId());
			identifiers.add(buffer);

			JsonObject components = doc["components"].to<JsonObject>();

			for (int i = 0; i < _digitalInputDiscretes.coils(); i++)
			{
				std::stringstream ss;
				ss << "DI" << i;
				JsonObject din = components[ss.str()].to<JsonObject>();
				din["platform"] = "binary_sensor";
				din["name"] = ss.str();
				din["payload_off"] = "Low";
				din["payload_on"] = "High";
				sprintf(buffer, "%X_%s", _iot.getUniqueId(), ss.str().c_str());
				din["unique_id"] = buffer;
				sprintf(buffer, "{{ value_json.%s }}", ss.str().c_str());
				din["value_template"] = buffer;
				din["icon"] = "mdi:switch";
			}
			for (int i = 0; i < AI_PINS; i++)
			{
				std::stringstream ss;
				ss << "AI" << i;		
				JsonObject ain = components[ss.str()].to<JsonObject>();
				ain["platform"] = "sensor";
				ain["name"] = ss.str();
				ain["unit_of_measurement"] = "%";
				sprintf(buffer, "%X_%s", _iot.getUniqueId(), ss.str().c_str());
				ain["unique_id"] = buffer;
				sprintf(buffer, "{{ value_json.%s }}", ss.str().c_str());
				ain["value_template"] = buffer;
				ain["icon"] = "mdi:lightning-bolt";
			}
			for (int i = 0; i < _digitalOutputCoils.coils(); i++)
			{
				std::stringstream ss;
				ss << "DO" << i;
				JsonObject dout = components[ss.str()].to<JsonObject>();
				dout["platform"] = "switch";
				dout["name"] = ss.str();
				dout["state_on"] = "On";
				dout["state_off"] = "Off";
				dout["command_topic"] = _iot.getRootTopicPrefix() + "/set/" + ss.str();
				sprintf(buffer, "%X_%s", _iot.getUniqueId(), ss.str().c_str());
				dout["unique_id"] = buffer;
				sprintf(buffer, "{{ value_json.%s }}", ss.str().c_str());
				dout["value_template"] = buffer;
				dout["icon"] = "mdi:valve";
			}

			sprintf(buffer, "%s/stat/readings", _iot.getRootTopicPrefix().c_str());
			doc["state_topic"] = buffer;
			sprintf(buffer, "%s/tele/LWT", _iot.getRootTopicPrefix().c_str());
			doc["availability_topic"] = buffer;
			doc["pl_avail"] = "Online";
			doc["pl_not_avail"] = "Offline";

			_iot.PublishHADiscovery(doc);
			_discoveryPublished = true;
		}
	}

	void PLC::onMqttMessage(char *topic, char *payload)
	{
		logd("onMqttMessage [%s] %s", topic, payload);
		std::string cmnd =_iot.getRootTopicPrefix() + "/set/";
		std::string fullPath = topic;
		if(strncmp(topic, cmnd.c_str(), cmnd.length()) == 0) 
		{
			// Handle set commands
			size_t lastSlash = fullPath.find_last_of('/');
			std::string dout;
			if (lastSlash != std::string::npos) 
			{
				dout = fullPath.substr(lastSlash + 1);
				logd("coil: %s: ", dout.c_str());
				for (int i = 0; i < DO_PINS; i++) //ToDo
				{
					std::stringstream ss;
					ss << "DO" << i;
					if (dout == ss.str())
					{
						String input = payload;
						input.toLowerCase();
						if (input == "on" || input == "high" || input == "1")
						{
							_Coils[i].Set(HIGH);
							logi("Write Coil %d HIGH", i);
						}
						else if (input == "off" || input == "low" || input == "0")
						{
							_Coils[i].Set(LOW);
							logi("Write Coil %d LOW", i);
						}
						else
						{
							logw("Write Coil %d invalid state: %s", i, input.c_str());
						}
						break;
					}
				}
			}
		} 
	}

// #pragma endregion MQTT
}