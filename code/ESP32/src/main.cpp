#include <Arduino.h>
#include <esp_task_wdt.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <time.h>
#include <ThreadController.h>
#include <Thread.h>
#include "Log.h"
#include "PLC.h"

using namespace ESP_PLC;

ThreadController _controller = ThreadController();
Thread *_workerThreadWaterLevelMonitor = new Thread();
PLC _plc = PLC();

void setup()
{
	Serial.begin(115200);
	while (!Serial) {}
	// Configure main worker thread
	_workerThreadWaterLevelMonitor->onRun([] () { _plc.Monitor(); });
	_workerThreadWaterLevelMonitor->setInterval(20);
	_controller.add(_workerThreadWaterLevelMonitor);
	_plc.setup();
	esp_task_wdt_init(WATCHDOG_TIMEOUT, true); 
	esp_task_wdt_add(NULL); // Add the current task to the watchdog timer
	logd("Setup Done");
}

void loop()
{
	_plc.Process();
	if (WiFi.isConnected())
	{
		_controller.run();
	}
	esp_task_wdt_reset(); // feed watchdog
}