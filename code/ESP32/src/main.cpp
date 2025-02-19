#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <time.h>
#include <ThreadController.h>
#include <Thread.h>
#include "Log.h"
#include "IOT.h"
#include "PLC.h"

using namespace ESP_PLC;

IOT _iot = IOT();
ThreadController _controller = ThreadController();
Thread *_workerThreadWaterLevelMonitor = new Thread();
PLC* _plc = new PLC();

void setup()
{
	Serial.begin(115200);
	while (!Serial) {}
	// Configure main worker thread
	_workerThreadWaterLevelMonitor->onRun([] () { _plc->Process(); });
	_workerThreadWaterLevelMonitor->setInterval(20);
	_controller.add(_workerThreadWaterLevelMonitor);
	_plc->setup(&_iot);
	_iot.Init(_plc);
	logd("Setup Done");
}

void loop()
{
	_iot.Run();
	if (WiFi.isConnected())
	{
		_controller.run();
	}
}