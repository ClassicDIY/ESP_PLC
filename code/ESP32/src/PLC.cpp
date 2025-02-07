#include <Arduino.h>
#include "ModbusServerTCPasync.h"
#include "IotWebConfOptionalGroup.h"
#include <IotWebConfTParameter.h>

#include "Log.h"
#include "HelperFunctions.h"

#include "PLC.h"

namespace ESP_PLC
{

	int digitalInputPins[DI_PINS] = {GPIO_NUM_12, GPIO_NUM_13, GPIO_NUM_14, GPIO_NUM_15, GPIO_NUM_16, GPIO_NUM_17, GPIO_NUM_18, GPIO_NUM_19, GPIO_NUM_21, GPIO_NUM_22, GPIO_NUM_23, GPIO_NUM_25};
	int digitalOutputPins[DO_PINS] = {GPIO_NUM_26, GPIO_NUM_27, GPIO_NUM_32, GPIO_NUM_33};
	int analogInputPins[AI_PINS] = {GPIO_NUM_34, GPIO_NUM_35, GPIO_NUM_36, GPIO_NUM_39};

	iotwebconf::ParameterGroup Gpio_group = iotwebconf::ParameterGroup("gpio", "GPIOs");
	iotwebconf::IntTParameter<int16_t> digitalInputsParam = iotwebconf::Builder<iotwebconf::IntTParameter<int16_t>>("digitalInputs").label("Digital Inputs").defaultValue(DI_PINS).min(0).max(DI_PINS).build();
	iotwebconf::IntTParameter<int16_t> analogInputsParam = iotwebconf::Builder<iotwebconf::IntTParameter<int16_t>>("analogInputs").label("Analog Inputs").defaultValue(AI_PINS).min(0).max(AI_PINS).build();

	iotwebconf::ParameterGroup Modbus_group = iotwebconf::ParameterGroup("modbus", "Modbus");
	iotwebconf::IntTParameter<int16_t> modbusPort = iotwebconf::Builder<iotwebconf::IntTParameter<int16_t>>("modbusPort").label("Modbus Port").defaultValue(502).build();
	iotwebconf::IntTParameter<int16_t> modbusID = iotwebconf::Builder<iotwebconf::IntTParameter<int16_t>>("modbusID").label("Modbus ID").defaultValue(1).min(0).max(247).build();

	PLC::PLC() : MBserver()
	{
		GPIO_NUM_0;
	}

	String PLC::getSettingsHTML()
	{
		String s;
		s += "GPIO:";
		s += "<ul>";
		s += htmlConfigEntry<int16_t>(digitalInputsParam.label, digitalInputsParam.value());

		char buffer[STR_LEN];
		s += "<li>Digital Input GPIOs: ";
		for (int i = 0; i < digitalInputsParam.value(); i++)
		{
			sprintf(buffer, "GPIO%d ", digitalInputPins[i]);
			s += buffer;
		}
		s += htmlConfigEntry<int16_t>(analogInputsParam.label, analogInputsParam.value());
		s += "</li><li>Analog Input GPIOs: ";
		for (int i = 0; i < analogInputsParam.value(); i++)
		{
			sprintf(buffer, "GPIO%d ", analogInputPins[i]);
			s += buffer;
		}
		s += "</li><li>Digital Output GPIOs: ";
		for (int i = 0; i < DO_PINS; i++)
		{
			sprintf(buffer, "GPIO%d ", digitalOutputPins[i]);
			s += buffer;
		}
		s += "</li></ul>";
		s += "Modbus:";
		s += "<ul>";
		s += htmlConfigEntry<int16_t>(modbusPort.label, modbusPort.value());
		s += htmlConfigEntry<int16_t>(modbusID.label, modbusID.value());
		s += "</ul>";

		return s;
	}

	iotwebconf::ParameterGroup *PLC::parameterGroup()
	{
		return &Gpio_group;
	}

	bool PLC::validate(iotwebconf::WebRequestWrapper *webRequestWrapper)
	{
		return true;
	}

	void PLC::setup(IOTServiceInterface *iot)
	{
		logd("setup");
		_iot = iot;

		Gpio_group.addItem(&digitalInputsParam);
		Gpio_group.addItem(&analogInputsParam);

		Modbus_group.addItem(&modbusPort);
		Modbus_group.addItem(&modbusID);
		Gpio_group.addItem(&Modbus_group);

		for (int i = 0; i < DI_PINS; i++)
		{
			pinMode(digitalInputPins[i], INPUT_PULLUP);
		}
		for (int i = 0; i < DO_PINS; i++)
		{
			pinMode(digitalOutputPins[i], OUTPUT);
		}
		for (int i = 0; i < AI_PINS; i++)
		{
			pinMode(analogInputPins[i], ANALOG);
		}
	}

	void PLC::onWiFiConnect()
	{
		if (!MBserver.isRunning())
		{
			MBserver.start(modbusPort.value(), 5, 0); // listen for modbus requests
		}
		for (int i = 0; i < DO_PINS; i++)
		{
			_digitalOutputCoils.set(i, digitalRead(_digitalOutputCoils[i]));
		}
		// READ_INPUT_REGISTER
		auto modbusFC04 = [this](ModbusMessage request) -> ModbusMessage
		{
			ModbusMessage response;
			uint16_t addr = 0;
			uint16_t words = 0;
			request.get(2, addr);
			request.get(4, words);
			logd("READ_INPUT_REGISTER %d %d[%d]", request.getFunctionCode(), addr, words);
			if ((addr + words) > AI_PINS)
			{
				logw("READ_INPUT_REGISTER error: %d", (addr + words));
				response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
			}
			else
			{
				response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)(words * 2));
				for (int i = addr; i < (addr + words); i++)
				{
					response.add((uint16_t)analogReadMilliVolts(analogInputPins[i]));
				}
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
			if ((start + numCoils) > DO_PINS)
			{
				logw("READ_COIL error: %d", (start + numCoils));
				response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
			}
			for (int i = 0; i < DO_PINS; i++)
			{
				_digitalOutputCoils.set(i, digitalRead(digitalOutputPins[i]));
			}
			vector<uint8_t> coilset = _digitalOutputCoils.slice(start, numCoils);
			response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)coilset.size(), coilset);
			return response;
		};

		// READ_DISCR_INPUT
		auto modbusFC02 = [this](ModbusMessage request) -> ModbusMessage
		{
			ModbusMessage response; // The Modbus message we are going to give back
			uint16_t start = 0;
			uint16_t numDiscretes = 0;
			request.get(2, start, numDiscretes);
			logd("READ_DISCR_INPUT %d %d[%d]", request.getFunctionCode(), start, numDiscretes);
			// Address overflow?
			if ((start + numDiscretes) > DI_PINS)
			{
				logw("READ_DISCR_INPUT error: %d", (start + numDiscretes));
				response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
			}
			for (int i = 0; i < DI_PINS; i++)
			{
				_digitalInputDiscretes.set(i, digitalRead(digitalInputPins[i]));
			}
			vector<uint8_t> coilset = _digitalInputDiscretes.slice(start, numDiscretes);
			response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)coilset.size(), coilset);
			return response;
		};

		// WRITE_COIL
		auto modbusFC05 = [this](ModbusMessage request) -> ModbusMessage
		{
			ModbusMessage response;
			// Request parameters are coil number and 0x0000 (OFF) or 0xFF00 (ON)
			uint16_t start = 0;
			uint16_t state = 0;
			request.get(2, start, state);
			logd("WRITE_COIL %d %d:%d", request.getFunctionCode(), start, state);
			// Is the coil number within the range of the coils?
			if (start <= DO_PINS)
			{
				// Looks like it. Is the ON/OFF parameter correct?
				if (state == 0x0000 || state == 0xFF00)
				{
					// Yes. We can set the coil
					if (_digitalOutputCoils.set(start, state))
					{
						digitalWrite(digitalOutputPins[start], state == 0xFF00 ? true : false);
						// All fine, coil was set.
						response = ECHO_RESPONSE;
					}
					else
					{
						// Setting the coil failed
						response.setError(request.getServerID(), request.getFunctionCode(), SERVER_DEVICE_FAILURE);
					}
				}
				else
				{
					// Wrong data parameter
					response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_VALUE);
				}
			}
			else
			{
				// Something was wrong with the coil number
				response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
			}
			// Return the response
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
			// Check the parameters so far
			if (numCoils && start + numCoils <= DO_PINS)
			{
				// Packed coils will fit in our storage
				if (numBytes == ((numCoils - 1) >> 3) + 1)
				{
					// Byte count seems okay, so get the packed coil bytes now
					vector<uint8_t> coilset;
					request.get(offset, coilset, numBytes);
					// Now set the coils
					if (_digitalOutputCoils.set(start, numCoils, coilset))
					{
						for (int i = 0; i < DO_PINS; i++)
						{
							digitalWrite(digitalOutputPins[i], _digitalOutputCoils[i]);
						}
						// All fine, return shortened echo response, like the standard says
						response.add(request.getServerID(), request.getFunctionCode(), start, numCoils);
					}
					else
					{
						// Oops! Setting the coils seems to have failed
						response.setError(request.getServerID(), request.getFunctionCode(), SERVER_DEVICE_FAILURE);
					}
				}
				else
				{
					// numBytes had a wrong value
					response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_VALUE);
				}
			}
			else
			{
				// The given set will not fit to our coil storage
				response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
			}
			return response;
		};
		MBserver.registerWorker(modbusID.value(), READ_INPUT_REGISTER, modbusFC04);
		MBserver.registerWorker(modbusID.value(), READ_COIL, modbusFC01);
		MBserver.registerWorker(modbusID.value(), READ_DISCR_INPUT, modbusFC02);
		MBserver.registerWorker(modbusID.value(), WRITE_COIL, modbusFC05);
		MBserver.registerWorker(modbusID.value(), WRITE_MULT_COILS, modbusFC0F);
	}

	uint16_t PLC::AddReading(uint16_t val)
	{
		uint16_t currentAvg = 0;
		if (_numberOfSummations > 0)
		{
			currentAvg = _rollingSum / _numberOfSummations;
		}
		if (_numberOfSummations < SAMPLESIZE)
		{
			_numberOfSummations++;
		}
		else
		{
			_rollingSum -= currentAvg;
		}
		_rollingSum += val;
		return _rollingSum / _numberOfSummations;
	}

	void PLC::Process()
	{
		JsonDocument doc;
		doc.clear();
		for (int i = 0; i < digitalInputsParam.value(); i++)
		{
			doc["GPIO_" + String(digitalInputPins[i])] = digitalRead(digitalInputPins[i]) ? "High" : "Low";
		}
		for (int i = 0; i < analogInputsParam.value(); i++)
		{
			doc["GPIO_" + String(analogInputPins[i])] = analogReadMilliVolts(analogInputPins[i]);
		}
		for (int i = 0; i < DO_PINS; i++)
		{
			doc["GPIO_" + String(digitalOutputPins[i])] = digitalRead(digitalOutputPins[i]);
		}
		String s;
		serializeJson(doc, s);
		if (_lastMessagePublished == s) // anything changed?
		{
			return;
		}
		_iot->Online();
		if (_iot->Publish("readings", s.c_str(), false))
		{
			_lastMessagePublished = s;
		}
		return;
	}

	void PLC::onMqttConnect(bool sessionPresent)
	{
		if (ReadyToPublish())
		{
			logd("Publishing discovery ");
			char buffer[STR_LEN];
			JsonDocument doc;
			JsonObject device = doc["device"].to<JsonObject>();
			device["name"] = _iot->getSubtopicName();
			device["sw_version"] = CONFIG_VERSION;
			device["manufacturer"] = "ClassicDIY";
			sprintf(buffer, "ESP32-Bit (%X)", _iot->getUniqueId());
			device["model"] = buffer;

			JsonObject origin = doc["origin"].to<JsonObject>();
			origin["name"] = TAG;

			JsonArray identifiers = device["identifiers"].to<JsonArray>();
			sprintf(buffer, "%X", _iot->getUniqueId());
			identifiers.add(buffer);

			JsonObject components = doc["components"].to<JsonObject>();

			for (int i = 0; i < digitalInputsParam.value(); i++)
			{
				sprintf(buffer, "GPIO_%d", digitalInputPins[i]);
				JsonObject din = components[buffer].to<JsonObject>();
				din["platform"] = "sensor";
				sprintf(buffer, "GPIO %d", digitalInputPins[i]);
				din["name"] = buffer;
				sprintf(buffer, "%X_GPIO_%d", _iot->getUniqueId(), digitalInputPins[i]);
				din["unique_id"] = buffer;
				sprintf(buffer, "{{ value_json.GPIO_%d }}", digitalInputPins[i]);
				din["value_template"] = buffer;
				din["icon"] = "mdi:switch";
			}
			for (int i = 0; i < analogInputsParam.value(); i++)
			{
				sprintf(buffer, "GPIO_%d", analogInputPins[i]);
				JsonObject ain = components[buffer].to<JsonObject>();
				ain["platform"] = "sensor";
				sprintf(buffer, "GPIO %d", analogInputPins[i]);
				ain["name"] = buffer;
				ain["unit_of_measurement"] = "mV";
				sprintf(buffer, "%X_GPIO_%d", _iot->getUniqueId(), analogInputPins[i]);
				ain["unique_id"] = buffer;
				sprintf(buffer, "{{ value_json.GPIO_%d }}", analogInputPins[i]);
				ain["value_template"] = buffer;
				ain["icon"] = "mdi:lightning-bolt";
			}
			for (int i = 0; i < DO_PINS; i++)
			{
				sprintf(buffer, "GPIO_%d", digitalOutputPins[i]);
				JsonObject dout = components[buffer].to<JsonObject>();
				dout["platform"] = "sensor";
				sprintf(buffer, "GPIO %d", digitalOutputPins[i]);
				dout["name"] = buffer;
				sprintf(buffer, "%X_GPIO_%d", _iot->getUniqueId(), digitalOutputPins[i]);
				dout["unique_id"] = buffer;
				sprintf(buffer, "{{ value_json.GPIO_%d }}", digitalOutputPins[i]);
				dout["value_template"] = buffer;
				dout["icon"] = "mdi:valve-open";
			}

			sprintf(buffer, "%s/stat/readings", _iot->getRootTopicPrefix().c_str());
			doc["state_topic"] = buffer;
			sprintf(buffer, "%s/tele/LWT", _iot->getRootTopicPrefix().c_str());
			doc["availability_topic"] = buffer;
			doc["pl_avail"] = "Online";
			doc["pl_not_avail"] = "Offline";

			_iot->PublishHADiscovery(doc);
			_discoveryPublished = true;
		}
	}

	void PLC::onMqttMessage(char *topic, JsonDocument &doc)
	{
		logd("onMqttMessage %s", topic);
		if (doc.containsKey("command"))
		{
			if (strcmp(doc["command"], "Write Coil") == 0)
			{
				int coil = doc["coil"];
				coil -= 1;
				if (coil >= 0 && coil < DO_PINS)
				{
					int state = strcmp(doc["state"], "HIGH") == 0 ? HIGH : LOW;
					digitalWrite(digitalOutputPins[coil], state);
					logi("Write Coil %d %d", coil, state);
				}
			}
		}
	}
}