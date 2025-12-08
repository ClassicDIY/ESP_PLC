#include <Arduino.h>
#ifdef UseLittleFS
#include <LittleFS.h>
#else
#include "app_script.js"
#include "app_style.css"
#endif
#include "Log.h"
#include "IOT.h"
#include "PLC.h"
#include "PLC.htm"

namespace CLASSICDIY {
static AsyncWebServer _asyncServer(ASYNC_WEBSERVER_PORT);
static AsyncWebSocket _webSocket("/ws_home");
IOT _iot = IOT();

#if defined(HasRS485) & defined(RTUBridge) & defined(HasModbus)
ModbusClientRTU _MBclientRTU(RS485_RTS, MODBUS_RTU_REQUEST_QUEUE_SIZE);
#endif

PLC::PLC() {}

PLC::~PLC() { logd("PLC destructor"); }

void PLC::Setup() {
   Init();
   _iot.Init(this, &_asyncServer);
   uint16_t coilCount = _useModbusBridge ? _coilCount + DO_PINS : DO_PINS;
   _digitalOutputCoils.Init(coilCount);
   uint16_t discreteCount = _useModbusBridge ? _discreteCount + DI_PINS : DI_PINS;
   _digitalInputDiscretes.Init(discreteCount, false);
   uint16_t analogOutputCount = _useModbusBridge ? _holdingCount + AO_PINS : AO_PINS;
   _analogOutputRegisters.Init(analogOutputCount);
   uint16_t analogInputCount = _useModbusBridge ? _inputCount + AI_PINS : AI_PINS;
   _analogInputRegisters.Init(analogInputCount);

   _asyncServer.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
      String page = home_html;
      page.replace("{n}", _iot.getThingName().c_str());
      page.replace("{v}", APP_VERSION);
      std::string s;
      for (int i = 0; i < _digitalInputDiscretes.coils(); i++) {
         s += "<div class='box' id=DI";
         s += std::to_string(i);
         s += "> DI";
         s += std::to_string(i);
         s += "</div>";
      }
      page.replace("{digitalInputs}", s.c_str());
      s.clear();
      for (int i = 0; i < _digitalOutputCoils.coils(); i++) {
         s += "<div class='box' id=DO";
         s += std::to_string(i);
         s += "> DO";
         s += std::to_string(i);
         s += "</div>";
      }
      page.replace("{digitalOutputs}", s.c_str());
      s.clear();
      for (int i = 0; i < _analogInputRegisters.size(); i++) {
         s += "<div class='box' id=AI";
         s += std::to_string(i);
         s += "> AI";
         s += std::to_string(i);
         s += "</div>";
      }
      page.replace("{analogInputs}", s.c_str());
      s.clear();
      for (int i = 0; i < _analogOutputRegisters.size(); i++) {
         s += "<div class='box' id=AO";
         s += std::to_string(i);
         s += "> AO";
         s += std::to_string(i);
         s += "</div>";
      }
#ifdef HasModbus
      char desc_buf[64];
      sprintf(desc_buf, "Modbus Coils: %d-%d", _iot.getMBBaseAddress(IOTypes::DigitalOutputs),
              _iot.getMBBaseAddress(IOTypes::DigitalOutputs) + _digitalOutputCoils.coils());
      page.replace("{digitalOutputDesc}", desc_buf);
      sprintf(desc_buf, "Modbus Input Registers: %d-%d", _iot.getMBBaseAddress(IOTypes::AnalogInputs),
              _iot.getMBBaseAddress(IOTypes::AnalogInputs) + _analogInputRegisters.size());
      page.replace("{analogInputDesc}", desc_buf);
      sprintf(desc_buf, "Modbus Discretes: %d-%d", _iot.getMBBaseAddress(IOTypes::DigitalInputs),
              _iot.getMBBaseAddress(IOTypes::DigitalInputs) + _digitalInputDiscretes.coils());
      page.replace("{digitalInputDesc}", desc_buf);
      sprintf(desc_buf, "Modbus Holding Registers: %d-%d", _iot.getMBBaseAddress(IOTypes::AnalogOutputs),
              _iot.getMBBaseAddress(IOTypes::AnalogOutputs) + _analogOutputRegisters.size());
      page.replace("{analogOutputDesc}", desc_buf);
#else
      page.replace("{digitalOutputDesc}", "");
      page.replace("{analogInputDesc}", "");
      page.replace("{digitalInputDesc}", "");
      page.replace("{analogOutputDesc}", "");
#endif

      page.replace("{analogOutputs}", s.c_str());
      request->send(200, "text/html", page);
   });
   _asyncServer.on("/appsettings", HTTP_GET, [this](AsyncWebServerRequest *request) {
      JsonDocument app;
      onSaveSetting(app);
      logd("HTTP_GET app_fields: %s", formattedJson(app).c_str());
      String s;
      serializeJson(app, s);
      request->send(200, "text/html", s);
   });
   _asyncServer.on(
       "/app_fields", HTTP_POST,
       [this](AsyncWebServerRequest *request) {
          // Called after all chunks are received
          logv("Full body received: %s", _bodyBuffer.c_str());
          // Parse JSON safely
          JsonDocument doc; // adjust size to expected payload
          DeserializationError err = deserializeJson(doc, _bodyBuffer);
          if (err) {
             logd("JSON parse failed: %s", err.c_str());
          } else {
             logd("HTTP_POST app_fields: %s", formattedJson(doc).c_str());
             onLoadSetting(doc);
          }
          request->send(200, "application/json", "{\"status\":\"ok\"}");
          _bodyBuffer = ""; // clear for next request
       },
       NULL, // file upload handler (not used here)
       [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
          logv("Chunk received: len=%d, index=%d, total=%d", len, index, total);
          // Append chunk to buffer
          _bodyBuffer.reserve(total); // reserve once for efficiency
          for (size_t i = 0; i < len; i++) {
             _bodyBuffer += (char)data[i];
          }
          if (index + len == total) {
             logd("Upload complete!");
          }
       });
   _asyncServer.addHandler(&_webSocket).addMiddleware([this](AsyncWebServerRequest *request, ArMiddlewareNext next) {
      // ws.count() is the current count of WS clients: this one is trying to upgrade its HTTP connection
      if (_webSocket.count() > 1) {
         // if we have 2 clients or more, prevent the next one to connect
         request->send(503, "text/plain", "Server is busy");
      } else {
         // process next middleware and at the end the handler
         next();
      }
   });
   _webSocket.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
      (void)len;
      if (type == WS_EVT_CONNECT) {
         _lastMessagePublished.clear(); // force a broadcast
         client->setCloseClientOnQueueFull(false);
         client->ping();
      } else if (type == WS_EVT_DISCONNECT) {
         // logi("Home Page Disconnected!");
      } else if (type == WS_EVT_ERROR) {
         loge("ws error");
      } else if (type == WS_EVT_PONG) {
         logd("ws pong");
         _lastMessagePublished.clear(); // force a broadcast
      }
   });
}

void PLC::onSaveSetting(JsonDocument &plc) {
#if AI_PINS > 0
   JsonArray rth = plc["conversions"].to<JsonArray>();
   for (int i = 0; i < AI_PINS; i++) {
      JsonObject obj = rth.add<JsonObject>();
      obj["minV"] = _AnalogSensors[i].minV();
      obj["minT"] = _AnalogSensors[i].minT();
      obj["maxV"] = _AnalogSensors[i].maxV();
      obj["maxT"] = _AnalogSensors[i].maxT();
   }
#endif
   // Bridge app settings
   plc["useModbusBridge"] = _useModbusBridge;
   plc["clientRTUBaud"] = _clientRTUBaud;
   plc["clientRTUParity"] = _clientRTUParity;
   plc["clientRTUStopBits"] = _clientRTUStopBits;

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

void PLC::onLoadSetting(JsonDocument &plc) {
#if AI_PINS > 0
   JsonArray conv = plc["conversions"].as<JsonArray>();
   int i = 0;
   for (JsonObject obj : conv) {
      if (i < AI_PINS) {
         _AnalogSensors[i].SetMinV(obj["minV"].as<float>());
         _AnalogSensors[i].SetMinT(obj["minT"].as<float>());
         _AnalogSensors[i].SetMaxV(obj["maxV"].as<float>());
         _AnalogSensors[i].SetMaxT(obj["maxT"].as<float>());
         i++;
      }
   }
#endif
   _useModbusBridge = plc["useModbusBridge"].isNull() ? false : plc["useModbusBridge"].as<bool>();
   _clientRTUBaud = plc["clientRTUBaud"].isNull() ? 9600 : plc["clientRTUBaud"].as<uint32_t>();
   _clientRTUParity = plc["clientRTUParity"].isNull() ? UART_PARITY_DISABLE : plc["clientRTUParity"].as<uart_parity_t>();
   _clientRTUStopBits = plc["clientRTUStopBits"].isNull() ? UART_STOP_BITS_1 : plc["clientRTUStopBits"].as<uart_stop_bits_t>();
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

String PLC::appTemplateProcessor(const String &var) {
   if (var == "style") {
      return String(app_style);
   }
   if (var == "app_fields") {
      return String(app_config_fields);
   }
   if (var == "appScript") {
      return String(rtuBridge_js);
   }
   #if defined(HasRS485) & defined(RTUBridge) & defined(HasModbus)
   if (var == "getClientRTUValues") {
      return String(getClientRTUValues);
   }
   if (var == "setClientRTUValues") {
      return String(setClientRTUValues);
   }
   if (var == "setModbusBridgeFieldset") {
      return String(setModbusBridgeFieldset);
   }
   if (var == "appshowMBFields") {
      return String("toggleMDBridgeFieldset();");
   }
   if (var == "modbusBridgeAppSettings") {
      return String(config_modbusBridge);
   }
   #endif
   // if (var == "validateInputs") {
   //    return String(app_validateInputs);
   // }
#if AI_PINS > 0
   if (var == "acf") {
      String appConvs;
      for (int i = 0; i < AI_PINS; i++) {
         String conv_flds(analog_conv_flds);
         conv_flds.replace("{An}", String(i));
         appConvs += conv_flds;
      }
      return appConvs;
   }
#endif
   if (var == "app_script_js") {
      return String(app_script_js);
   }
   logd("Did not find app template for: %s", var.c_str());
   return String("");
}

void PLC::CleanUp() {
   _webSocket.cleanupClients(); // cleanup disconnected clients or too many clients
#if defined(HasRS485) & defined(RTUBridge) & defined(HasModbus)
   // read modbus bridge outputs to compare with current set.
   if (_iot.getNetworkState() == OnLine && _useModbusBridge) {
      // fetch the actual coil info to compare with coilset
      if (_coilCount > 0) {
         ModbusMessage forward;
         uint8_t err = forward.setMessage(_coilID, READ_COIL, _coilAddress, _coilCount);
         if (err == SUCCESS) {
            Modbus::Error error = SendToModbusBridgeAsync(forward);
            if (error != SUCCESS) {
               logd("Error forwarding READ_COIL to modbus bridge device Id:%d Error: %02X - %s", _coilID, error,
                    (const char *)ModbusError(error));
            }
         }
      }
      // fetch the actual register info to compare with registerset
      if (_holdingCount > 0) {
         ModbusMessage forward;
         uint8_t err = forward.setMessage(_holdingID, READ_HOLD_REGISTER, _holdingAddress, _holdingCount);
         if (err == SUCCESS) {
            Modbus::Error error = SendToModbusBridgeAsync(forward);
            if (error != SUCCESS) {
               logd("Error forwarding WRITE_MULT_REGISTERS to modbus bridge device Id:%d Error: %02X - %s", _holdingID, error,
                    (const char *)ModbusError(error));
            }
         }
      }
   }
#endif
}

void PLC::Process() {
   _iot.Run();
   Run(); // base class

   if (_iot.getNetworkState() == OnLine) {
      JsonDocument doc;
      doc.clear();
      for (int i = 0; i < _digitalInputDiscretes.coils(); i++) {
         std::stringstream ss;
         ss << "DI" << i;
         doc[ss.str()] = _digitalInputDiscretes[i] ? "High" : "Low";
      }
      for (int i = 0; i < _analogInputRegisters.size(); i++) {
         std::stringstream ss;
         ss << "AI" << i;
         doc[ss.str()] = _analogInputRegisters[i];
      }
      for (int i = 0; i < _digitalOutputCoils.coils(); i++) {
         std::stringstream ss;
         ss << "DO" << i;
         doc[ss.str()] = _digitalOutputCoils[i] ? "On" : "Off";
      }
      for (int i = 0; i < _analogOutputRegisters.size(); i++) {
         std::stringstream ss;
         ss << "AO" << i;
         doc[ss.str()] = _analogOutputRegisters[i];
      }
      String s;
      serializeJson(doc, s);
      DeserializationError err = deserializeJson(doc, s);
      if (err) {
         loge("deserializeJson() failed: %s", err.c_str());
      }
      if (_lastMessagePublished == s) // anything changed?
      {
         return;
      }
#ifdef HasMQTT
      _iot.Publish("readings", s.c_str(), false);
#endif
      _lastMessagePublished = s;
      _webSocket.textAll(s);
      logv("Published readings: %s", s.c_str());
#if defined(HasRS485) & defined(RTUBridge) & defined(HasModbus)
      unsigned long now = millis();
      if (MODBUS_POLL_RATE < now - _lastModbusPollTime) {
         _lastModbusPollTime = now;

         if (_useModbusBridge) {
            if (_discreteCount > 0) {
               ModbusMessage forward;
               uint8_t err = forward.setMessage(_discreteID, READ_DISCR_INPUT, _discreteAddress, _discreteCount);
               if (err == SUCCESS) {
                  Modbus::Error error = SendToModbusBridgeAsync(forward);
                  if (error != SUCCESS) {
                     logd("Error forwarding FC02 to modbus bridge device Id:%d Error: %02X - %s", _discreteID, error,
                          (const char *)ModbusError(error));
                  }
               } else {
                  loge("poll discrete error: 0X%x", err);
               }
            }
            if (_inputCount > 0) {
               ModbusMessage forward;
               uint8_t err = forward.setMessage(_inputID, READ_INPUT_REGISTER, _inputAddress, _inputCount);
               if (err == SUCCESS) {
                  Modbus::Error error = SendToModbusBridgeAsync(forward);
                  if (error != SUCCESS) {
                     logd("Error forwarding FC03 to modbus bridge device Id:%d Error: %02X - %s", _inputID, error,
                          (const char *)ModbusError(error));
                  }
               } else {
                  loge("poll holding error: 0X%x", err);
               }
            }
         }
      }
#endif
   }
}

void PLC::onNetworkState(NetworkState state) {
   _networkState = state;
   if (state == OnLine) {

#ifdef HasModbus

      // READ_INPUT_REGISTER
      auto modbusFC04 = [this](ModbusMessage request) -> ModbusMessage {
         ModbusMessage response;
         uint16_t addr = 0;
         uint16_t words = 0;
         request.get(2, addr);
         request.get(4, words);
         logd("READ_INPUT_REGISTER %d %d[%d]", request.getFunctionCode(), addr, words);
         addr -= _iot.getMBBaseAddress(AnalogInputs);
         if ((addr + words) <= _analogInputRegisters.size()) {
            response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)(words * 2));
            for (int i = addr; i < (addr + words); i++) {
               response.add((uint16_t)_analogInputRegisters[i]);
            }
         } else {
            logw("READ_INPUT_REGISTER Address overflow: %d", (addr + words));
            response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
         }
         return response;
      };

      // READ_DISCR_INPUT
      auto modbusFC02 = [this](ModbusMessage request) -> ModbusMessage {
         ModbusMessage response;
         uint16_t addr = 0;
         uint16_t numdisc = 0;
         request.get(2, addr, numdisc);
         logd("READ_DISCR_INPUT FC%d %d[%d]", request.getFunctionCode(), addr, numdisc);
         addr -= _iot.getMBBaseAddress(DigitalInputs);
         if ((addr + numdisc) <= _digitalInputDiscretes.coils()) {
            vector<uint8_t> discreteSet = _digitalInputDiscretes.slice(addr, numdisc);
            response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)discreteSet.size(), discreteSet);
         } else {
            logw("READ_DISCR_INPUT Address overflow: %d", (addr + numdisc));
            response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
         }
         return response;
      };

      // READ_COIL
      auto modbusFC01 = [this](ModbusMessage request) -> ModbusMessage {
         ModbusMessage response;
         uint16_t addr = 0;
         uint16_t numCoils = 0;
         request.get(2, addr, numCoils);
         logd("READ_COIL %d %d[%d]", request.getFunctionCode(), addr, numCoils);
         // Address overflow?
         addr -= _iot.getMBBaseAddress(DigitalOutputs);
         if ((addr + numCoils) <= _digitalOutputCoils.coils()) {
            vector<uint8_t> coilset = _digitalOutputCoils.slice(addr, numCoils);
            response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)coilset.size(), coilset);
         } else {
            logw("READ_COIL Address overflow: %d", (addr + numCoils));
            response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
         }
         return response;
      };

      // WRITE_COIL
      auto modbusFC05 = [this](ModbusMessage request) -> ModbusMessage {
         ModbusMessage response;
         // Request parameters are coil number and 0x0000 (OFF) or 0xFF00 (ON)
         uint16_t addr = 0;
         uint16_t state = 0;
         request.get(2, addr, state);
         logd("WRITE_COIL %d %d:%d", request.getFunctionCode(), addr, state);
         addr -= _iot.getMBBaseAddress(DigitalOutputs);
         // Is the coil number within the range of the coils?
         if (addr < _digitalOutputCoils.coils()) {
            // Looks like it. Is the ON/OFF parameter correct?
            if (state == 0x0000 || state == 0xFF00) {
               if (_digitalOutputCoils.set(addr, state)) {
                  if (addr < DO_PINS) {
#if DO_PINS > 0
                     // Set the native coil
                     SetRelay(addr, state == 0xFF00 ? HIGH : LOW);
                     response = ECHO_RESPONSE;
#endif
#if defined(HasRS485) & defined(RTUBridge) & defined(HasModbus)
                  } else // bridge coil
                  {
                     addr -= DO_PINS; // start of bridge coils
                     ModbusMessage forward;
                     Error err = forward.setMessage(_coilID, request.getFunctionCode(), addr, state);
                     if (err == SUCCESS) {
                        err = SendToModbusBridgeAsync(forward);
                        if (err == SUCCESS) {
                           // All fine, return shortened echo response, like the standard says
                           response = ECHO_RESPONSE;
                        } else {
                           ModbusError e(err);
                           logd("Error forwarding FC%d to modbus bridge device Id:%d Error:  %02X - %s", request.getFunctionCode(), _coilID,
                                (int)e, (const char *)e);
                           response.setError(request.getServerID(), request.getFunctionCode(), (Error)e);
                        }
                     } else {
                        loge("ModbusMessage Write coil error: 0X%x", err);
                        response.setError(request.getServerID(), request.getFunctionCode(), err);
                     }
#endif
                  }
               } else {
                  response.setError(request.getServerID(), request.getFunctionCode(), SERVER_DEVICE_FAILURE);
               }
            } else {
               // Wrong data parameter
               response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_VALUE);
            }
         } else {
            // Wrong data parameter
            response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
         }
         return response;
      };

      // WRITE_MULT_COILS
      auto modbusFC0F = [this](ModbusMessage request) -> ModbusMessage {
         ModbusMessage response;
         // Request parameters are first coil to be set, number of coils, number of bytes and packed coil bytes
         uint16_t addr = 0;
         uint16_t numCoils = 0;
         uint8_t numBytes = 0;
         uint16_t offset = 2; // Parameters addr after serverID and FC
         offset = request.get(offset, addr, numCoils, numBytes);
         logd("WRITE_MULT_COILS %d %d[%d]", request.getFunctionCode(), addr, numCoils);
         addr -= _iot.getMBBaseAddress(DigitalOutputs);
         if ((addr + numCoils) <= _digitalOutputCoils.coils()) {
            // Packed coils will fit in our storage
            if (numBytes == ((numCoils - 1) >> 3) + 1) {
               // Byte count seems okay, so get the packed coil bytes now
               vector<uint8_t> coilset;
               request.get(offset, coilset, numBytes);
               logd("offset: %d coilset: %d numCoils: %d numBytes: %d addr: %d", offset, coilset.size(), numCoils, numBytes, addr);
               // Now set the coils
               if (_digitalOutputCoils.set(addr, numCoils, coilset)) {
#if DO_PINS > 0
                  // set native DO pins
                  for (int i = 0; i < DO_PINS; i++) {
                     SetRelay(i, _digitalOutputCoils[i] ? HIGH : LOW);
                  }
                  response = ECHO_RESPONSE;
#endif
#if defined(HasRS485) & defined(RTUBridge) & defined(HasModbus)
                  numCoils -= DO_PINS;
                  CoilSet bridgeCoilset = _digitalOutputCoils.slice(DO_PINS, numCoils); // bridge coils are forwarded to the modbus bridge
                  ModbusMessage forward;
                  Error err = forward.setMessage(_coilID, request.getFunctionCode(), 0, bridgeCoilset.coils(), bridgeCoilset.size(),
                                                 bridgeCoilset.data());
                  if (err == SUCCESS) {
                     err = SendToModbusBridgeAsync(forward);
                     if (err == SUCCESS) {
                        response = ECHO_RESPONSE; // All fine, return shortened echo response, like the standard says
                     } else {
                        ModbusError e(err);
                        logd("Error forwarding FC%d to modbus bridge device Id:%d Error:  %02X - %s", request.getFunctionCode(), _coilID, (int)e,
                             (const char *)e);
                        response.setError(request.getServerID(), request.getFunctionCode(), (Error)e);
                     }
                  } else {
                     loge("ModbusMessage Write multiple coils error: 0X%x", err);
                     response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_VALUE);
                  }
#endif
               } else {
                  logd("SERVER_DEVICE_FAILURE Setting the coils seems to have failed");
                  response.setError(request.getServerID(), request.getFunctionCode(), SERVER_DEVICE_FAILURE);
               }
            } else {
               logd("ILLEGAL_DATA_VALUE numBytes had a wrong value");
               response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_VALUE);
            }
         } else {
            logd("ILLEGAL_DATA_VALUE The given set will not fit to our coil storage");
            response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
         }
         return response;
      };

      // READ_HOLD_REGISTER
      auto modbusFC03 = [this](ModbusMessage request) -> ModbusMessage {
         ModbusMessage response;
         uint16_t addr = 0;
         uint16_t words = 0;
         request.get(2, addr);
         request.get(4, words);
         logd("READ_HOLD_REGISTER FC%d %d[%d]", request.getFunctionCode(), addr, words);
         addr -= _iot.getMBBaseAddress(AnalogOutputs);
         if ((addr + words) <= _analogOutputRegisters.size()) {
            response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)(words * 2));
            for (int i = addr; i < (addr + words); i++) {
               response.add((uint16_t)_analogOutputRegisters[i]);
            }
         } else {
            logw("READ_HOLD_REGISTER  Address overflow: %d", (addr + words));
            response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
         }
         return response;
      };

      // WRITE_HOLD_REGISTER
      auto modbusFC06 = [this](ModbusMessage request) -> ModbusMessage {
         ModbusMessage response;
         uint16_t addr = 0;  // register address
         uint16_t value = 0; // register value
         request.get(2, addr, value);
         logd("WRITE_HOLD_REGISTER FC%d %d[%d]", request.getFunctionCode(), addr, value);
         addr -= _iot.getMBBaseAddress(AnalogOutputs);
         if (addr < _analogOutputRegisters.size()) {
            if (_analogOutputRegisters.set(addr, value)) {
               if (addr < AO_PINS) {
#if AO_PINS > 0
                  // Set the native coil
                  _PWMOutputs[addr].SetDutyCycle(value);
                  response = ECHO_RESPONSE;
#endif
#if defined(HasRS485) & defined(RTUBridge) & defined(HasModbus)
               } else // bridge coil
               {
                  addr -= AO_PINS; // start of bridge registers
                  ModbusMessage forward;
                  Error err = forward.setMessage(_holdingID, request.getFunctionCode(), addr, value);
                  if (err == SUCCESS) {
                     err = SendToModbusBridgeAsync(forward);
                     if (err == SUCCESS) {
                        // All fine, return shortened echo response, like the standard says
                        response = ECHO_RESPONSE;
                     } else {
                        ModbusError e(err);
                        logd("Error forwarding FC%d to modbus bridge device Id:%d Error:  %02X - %s", request.getFunctionCode(), _holdingID,
                             (int)e, (const char *)e);
                        response.setError(request.getServerID(), request.getFunctionCode(), (Error)e);
                     }
                  } else {
                     loge("ModbusMessage write holding error: 0X%x", err);
                     response.setError(request.getServerID(), request.getFunctionCode(), err);
                  }
#endif
               }
            } else {
               response.setError(request.getServerID(), request.getFunctionCode(), SERVER_DEVICE_FAILURE);
            }
         } else {
            logd("ILLEGAL_DATA_VALUE The given set will not fit to our storage");
            response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
         }
         return response;
      };

      // WRITE_MULT_REGISTERS
      auto modbusFC10 = [this](ModbusMessage request) -> ModbusMessage {
         ModbusMessage response;
         uint16_t addr = 0;
         uint16_t numRegs = 0;
         uint8_t numBytes = 0;
         uint16_t offset = 2; // Parameters start after serverID and FC
         offset = request.get(offset, addr, numRegs, numBytes);
         logd("WRITE_MULT_REGISTERS %d %d[%d] (%d bytes)", request.getFunctionCode(), addr, numRegs, numBytes);
         addr -= _iot.getMBBaseAddress(AnalogOutputs);
         if (addr < _analogOutputRegisters.size()) {
            if (numRegs == numBytes / 2) {
               vector<uint8_t> regset;
               request.get(offset, regset, numBytes);
               logd("offset: %d regset: %d numRegisters: %d numBytes: %d addr: %d", offset, regset.size(), numRegs, numBytes, addr);
               // Now set the registers
               bool rVal = _analogOutputRegisters.set(addr, numRegs, (uint8_t *)regset.data());
               if (rVal) {
#if AO_PINS > 0
                  // set native AO pins
                  for (int i = 0; i < AO_PINS; i++) {
                     _PWMOutputs[i].SetDutyCycle(_analogOutputRegisters[i]);
                  }
                  response = ECHO_RESPONSE;
#endif
#if defined(HasRS485) & defined(RTUBridge) & defined(HasModbus)
                  numRegs -= AO_PINS; // start of bridge registers
                  RegisterSet bridgeRegset = _analogOutputRegisters.slice(AO_PINS, numRegs);
                  // bridge registers are forwarded to the modbus bridge
                  ModbusMessage forward;
                  Error err = forward.setMessage(_holdingID, request.getFunctionCode(), 0, bridgeRegset.size(), bridgeRegset.size() * 2,
                                                 bridgeRegset.data());
                  if (err == SUCCESS) {
                     err = SendToModbusBridgeAsync(forward);
                     if (err == SUCCESS) {
                        response = ECHO_RESPONSE; // All fine, return shortened echo response, like the standard says
                     } else {
                        ModbusError e(err);
                        logd("Error forwarding FC%d to modbus bridge device Id:%d Error:  %02X - %s", request.getFunctionCode(), _holdingID,
                             (int)e, (const char *)e);
                        response.setError(request.getServerID(), request.getFunctionCode(), (Error)e);
                     }
                  } else {
                     ModbusError e(err);
                     loge("ModbusMessage Write multiple registers error: %02X - %s", (int)e, (const char *)e);
                     response.setError(request.getServerID(), request.getFunctionCode(), err);
                  }
#endif
               } else {
                  logd("SERVER_DEVICE_FAILURE Setting the registers seems to have failed");
                  response.setError(request.getServerID(), request.getFunctionCode(), SERVER_DEVICE_FAILURE);
               }
            } else {
               logd("ILLEGAL_DATA_VALUE numBytes had a wrong value");
               response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_VALUE);
            }
         } else {
            logd("ILLEGAL_DATA_VALUE The given set will not fit to our register storage");
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

#if defined(HasRS485) & defined(RTUBridge) & defined(HasModbus)
      if (_useModbusBridge) {
         _MBclientRTU.setTimeout(MODBUS_RTU_TIMEOUT);
         _MBclientRTU.begin(Serial2);
         _MBclientRTU.useModbusRTU();
         _MBclientRTU.onDataHandler([this](ModbusMessage response, uint32_t token) {
            logv("RTU Response: serverID=%d, FC=%d, Token=%08X, length=%d", response.getServerID(), response.getFunctionCode(), token,
                 response.size());
            return onModbusMessage(response);
         });
         _MBclientRTU.onErrorHandler([this](Modbus::Error mbError, uint32_t token) {
            logd("Modbus RTU (Token: %d) Error response: %02X - %s", token, (int)mbError, (const char *)ModbusError(mbError));
            if (_MBclientRTU.pendingRequests() > 2) {
               logd("Modbus RTU clearing queue!");
               _MBclientRTU.clearQueue();
               Serial2.flush();
            }
            return true;
         });
      }
#endif
#endif
   }
}

#ifdef Has_OLED
void PLC::update(const char *mode, const char *detail) { _oled.update(_iot.getThingName().c_str(), mode, detail); }
void PLC::update(const char *mode, int count) { _oled.update(_iot.getThingName().c_str(), mode, count); }
#endif

#ifdef HasModbus
bool PLC::onModbusMessage(ModbusMessage &msg) {
   bool rval = false;

   switch (msg.getFunctionCode()) {
   case READ_DISCR_INPUT:
      rval = _digitalInputDiscretes.set(DI_PINS, _discreteCount, (uint8_t *)msg.data() + 3);
      break;
   case READ_HOLD_REGISTER: {
      RegisterSet receivedRegisters;
      RegisterSet slicedRegisters = _analogOutputRegisters.slice(AO_PINS, _holdingCount);
      receivedRegisters.Init(_holdingCount);
      receivedRegisters.set(0, _holdingCount, (uint8_t *)(msg.data() + 3));
      if (receivedRegisters != slicedRegisters) {
         logd("Make sure holding registers have the same value as _analogOutputRegisters");
         ModbusMessage forward;
         Error err =
             forward.setMessage(_holdingID, WRITE_MULT_REGISTERS, 0, slicedRegisters.size(), slicedRegisters.size() * 2, slicedRegisters.data());
         if (err == SUCCESS) {
            err = SendToModbusBridgeAsync(forward);
            if (err != SUCCESS) {
               ModbusError e(err);
               logd("Error forwarding Holding registers to modbus bridge device Id:%d Error:  %02X - %s", _holdingID, (int)e, (const char *)e);
            }
         }
      }
   } break;
   case READ_COIL: {
      // verify bridge coils have the same value as _digitalOutputCoils
      CoilSet receivedCoils;
      CoilSet slicedCoils = _digitalOutputCoils.slice(DO_PINS, _coilCount);
      receivedCoils.Init(_coilCount);
      rval = receivedCoils.set(0, _coilCount, (uint8_t *)msg.data() + 3);
      if (receivedCoils != slicedCoils) {
         logd("Make sure coils have the same value as _digitalOutputCoils");
         ModbusMessage forward;
         Error err = forward.setMessage(_coilID, WRITE_MULT_COILS, 0, slicedCoils.coils(), slicedCoils.size(), slicedCoils.data());
         if (err == SUCCESS) {
            err = SendToModbusBridgeAsync(forward);
            if (err != SUCCESS) {
               ModbusError e(err);
               logd("Error forwarding Coils to modbus bridge device Id:%d Error:  %02X - %s", _coilID, (int)e, (const char *)e);
            }
         }
      }
   } break;
   case READ_INPUT_REGISTER:
      uint16_t offs = 3; // First value is on pos 3, after server ID, function code and length byte
      uint16_t values[_inputCount];
      for (uint8_t i = 0; i < _inputCount; ++i) {
         offs = msg.get(offs, values[i]);
         if (values[i] != 0) {
            uint16_t v = (values[i] + 500) / 1000; // convert to mA rounded
            v -= 4;                                // convert to %
            v *= 625;
            v /= 100;
            values[i] = v;
         }
      }
      rval = _analogInputRegisters.set(AI_PINS, _inputCount, values);
      break;
   }
   return rval;
}

#endif
Modbus::Error PLC::SendToModbusBridgeAsync(ModbusMessage &request) {
   Modbus::Error mbError = INVALID_SERVER;
#if defined(HasRS485) & defined(RTUBridge) & defined(HasModbus)
   if (_useModbusBridge) {
      uint32_t token = nextToken();
      logv("SendToModbusBridge Token=%08X FC%d", token, request.getFunctionCode());
      if (_MBclientRTU.pendingRequests() < MODBUS_RTU_REQUEST_QUEUE_SIZE) {
         mbError = _MBclientRTU.addRequest(request, token);
         mbError = SUCCESS;
      } else {
         mbError = REQUEST_QUEUE_FULL;
      }
      delay(100);
   }
#endif
   return mbError;
}
#ifdef HasMQTT

void PLC::onMqttConnect() {
   if (!_discoveryPublished) {
      for (int i = 0; i < _digitalInputDiscretes.coils(); i++) {
         std::stringstream ss;
         ss << "DI" << i;
         if (PublishDiscoverySub(DigitalInputs, ss.str().c_str(), nullptr, "mdi:switch") == false) {
            return; // try later
         }
      }
      for (int i = 0; i < _digitalOutputCoils.coils(); i++) {
         std::stringstream ss;
         ss << "DO" << i;
         if (PublishDiscoverySub(DigitalOutputs, ss.str().c_str(), nullptr, "mdi:valve") == false) {
            return; // try later
         }
      }
      for (int i = 0; i < _analogInputRegisters.size(); i++) {
         std::stringstream ss;
         ss << "AI" << i;
         if (PublishDiscoverySub(AnalogInputs, ss.str().c_str(), "%", "mdi:lightning-bolt") == false) {
            return; // try later
         }
      }
      for (int i = 0; i < _analogOutputRegisters.size(); i++) {
         std::stringstream ss;
         ss << "AO" << i;
         if (PublishDiscoverySub(AnalogOutputs, ss.str().c_str(), nullptr, nullptr) == false) {
            return; // try later
         }
      }
      _discoveryPublished = true;
   }
}

boolean PLC::PublishDiscoverySub(IOTypes type, const char *entityName, const char *unit_of_meas, const char *icon) {
   String topic = HOME_ASSISTANT_PREFIX;
   switch (type) {
   case DigitalOutputs:
      topic += "/switch/";
      break;
   case AnalogOutputs:
      topic += "/number/";
      break;
   case DigitalInputs:
      topic += "/sensor/";
      break;
   case AnalogInputs:
      topic += "/sensor/";
      break;
   }
   topic += String(_iot.getUniqueId());
   topic += "/";
   topic += entityName;
   topic += "/config";

   JsonDocument payload;
   payload["platform"] = "mqtt";
   payload["name"] = entityName;
   payload["unique_id"] = String(_iot.getUniqueId()) + "_" + String(entityName);
   payload["value_template"] = ("{{ value_json." + String(entityName) + " }}").c_str();
   payload["state_topic"] = _iot.getRootTopicPrefix().c_str() + String("/stat/readings");
   if (type == DigitalOutputs) {
      payload["command_topic"] = _iot.getRootTopicPrefix().c_str() + String("/set/") + String(entityName);
      payload["state_on"] = "On";
      payload["state_off"] = "Off";
   } else if (type == AnalogOutputs) {
      payload["command_topic"] = _iot.getRootTopicPrefix().c_str() + String("/set/") + String(entityName);
      payload["min"] = 0;
      payload["max"] = 65535;
      payload["step"] = 1;
   } else if (type == DigitalInputs) {
      payload["payload_off"] = "Low";
      payload["payload_on"] = "High";
   }
   payload["availability_topic"] = _iot.getRootTopicPrefix().c_str() + String("/tele/LWT");
   payload["payload_available"] = "Online";
   payload["payload_not_available"] = "Offline";
   if (unit_of_meas) {
      payload["unit_of_measurement"] = unit_of_meas;
   }
   if (icon) {
      payload["icon"] = icon;
   }

   char buffer[STR_LEN];
   JsonObject device = payload["device"].to<JsonObject>();
   device["name"] = _iot.getThingName().c_str();
   device["sw_version"] = APP_VERSION;
   device["manufacturer"] = "ClassicDIY";
   sprintf(buffer, "%s (%X)", TAG, _iot.getUniqueId());
   device["model"] = buffer;
   JsonArray identifiers = device["identifiers"].to<JsonArray>();
   sprintf(buffer, "%X", _iot.getUniqueId());
   identifiers.add(buffer);

   logd("Discovery => topic: %s", topic.c_str());
   return _iot.PublishMessage(topic.c_str(), payload, true);
}

void PLC::onMqttMessage(char *topic, char *payload) {
   logd("onMqttMessage [%s] %s", topic, payload);
   String prefix = _iot.getRootTopicPrefix();
   prefix += "/set/";
   std::string cmnd = prefix.c_str();
   std::string fullPath = topic;
   if (strncmp(topic, cmnd.c_str(), cmnd.length()) == 0) {
      // Handle set commands
      size_t lastSlash = fullPath.find_last_of('/');
      std::string dout;
      if (lastSlash != std::string::npos) {
         dout = fullPath.substr(lastSlash + 1);
         if (dout[0] == 'D') // coils?
         {
            logd("coil: %s: ", dout.c_str());
            for (int i = 0; i < _digitalOutputCoils.coils(); i++) {
               std::stringstream ss;
               ss << "DO" << i;
               if (dout == ss.str()) {
                  String input = payload;
                  input.toLowerCase();
                  if (input == "on" || input == "high" || input == "1") {
                     _digitalOutputCoils.set(i, true);
                     logi("Write Coil %d HIGH", i);
                  } else if (input == "off" || input == "low" || input == "0") {
                     _digitalOutputCoils.set(i, false);
                     logi("Write Coil %d LOW", i);
                  } else {
                     logw("Write Coil %d invalid state: %s", i, input.c_str());
                  }
                  break;
               }
            }
#if DO_PINS > 0
            // set native DO pins
            for (int j = 0; j < DO_PINS; j++) {
               SetRelay(j, _digitalOutputCoils[j] ? HIGH : LOW);
            }
#endif
         } else if (dout[0] == 'A') // registers?
         {
            logd("gerister: %s: ", dout.c_str());
            for (int i = 0; i < _analogOutputRegisters.size(); i++) {
               std::stringstream ss;
               ss << "AO" << i;
               if (dout == ss.str()) {
                  String input = payload;
                  input.toLowerCase();
                  logd("Analog output value: %s", input.c_str());
                  _analogOutputRegisters.set(i, atoi(input.c_str()));
                  break;
               }
            }
#if AO_PINS > 0
            // set native AO pins
            for (int i = 0; i < AO_PINS; i++) {
               _PWMOutputs[i].SetDutyCycle(_analogOutputRegisters[i]);
            }
#endif
         }
      }
   }
}
#endif
} // namespace CLASSICDIY