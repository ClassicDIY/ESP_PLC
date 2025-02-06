#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <WebServer.h>
#include <time.h>
#include <ThreadController.h>
#include <Thread.h>
#include "ModbusServerTCPasync.h"
#include "Log.h"
#include "IOT.h"
#include "PLC.h"


#define WATCHDOG_TIMER 600000 // time in ms to trigger the watchdog

using namespace ESP_PLC;

WebServer _webServer(80);
IOT _iot = IOT(&_webServer);
ThreadController _controller = ThreadController();
Thread *_workerThreadWaterLevelMonitor = new Thread();
PLC* _plc = new PLC();

hw_timer_t *_watchdogTimer = NULL;

void IRAM_ATTR resetModule()
{
	// ets_printf("watchdog timer expired - rebooting\n");
	esp_restart();
}

void init_watchdog()
{
	if (_watchdogTimer == NULL)
	{
		_watchdogTimer = timerBegin(0, 80, true);					   // timer 0, div 80
		timerAttachInterrupt(_watchdogTimer, &resetModule, true);	   // attach callback
		timerAlarmWrite(_watchdogTimer, WATCHDOG_TIMER * 1000, false); // set time in us
		timerAlarmEnable(_watchdogTimer);							   // enable interrupt
	}
}

void feed_watchdog()
{
	if (_watchdogTimer != NULL)
	{
		timerWrite(_watchdogTimer, 0); // feed the watchdog
	}
}

void runPLC_Monitor()
{
	_plc->Process();
}

void setup()
{
	Serial.begin(115200);
	while (!Serial) {}
	// Configure main worker thread
	_workerThreadWaterLevelMonitor->onRun(runPLC_Monitor);
	_workerThreadWaterLevelMonitor->setInterval(20);
	_controller.add(_workerThreadWaterLevelMonitor);
	_plc->setup(&_iot);
	_iot.Init(_plc);
	init_watchdog();

	logd("Setup Done");
}

void loop()
{
	_iot.Run();
	if (WiFi.isConnected())
	{
		_controller.run();
	}
	feed_watchdog();
}