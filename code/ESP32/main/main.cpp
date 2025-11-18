#include <Arduino.h>
#include <esp_task_wdt.h>
#include <esp_system.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <time.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#ifdef Has_OLED_Display
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#endif
#include <Thread.h>
#include <ThreadController.h>
#include "main.h"
#include "Log.h"
#include "PLC.h"

using namespace CLASSICDIY;

#ifdef HasRTC
#include "RTClib.h"
RTC_PCF8563 rtc;
#endif

static Main my_main;
PLC _plc = PLC();
Adafruit_ADS1115 ads; /* Use this for the 16-bit version */
ThreadController _controller = ThreadController();
Thread *_workerThread1 = new Thread();
Thread *_workerThread2 = new Thread();
Thread *_workerThread3 = new Thread();

esp_err_t Main::setup() {
   // wait for Serial to connect, give up after 5 seconds, USB may not be connected
   unsigned long start = millis();
   Serial.begin(115200);
   while (!Serial) {
      if (5000 < millis() - start) {
         break;
      }
   }
   esp_err_t ret = ESP_OK;

   logd("------------ESP32 specifications ---------------");
   logd("Chip Model: %s", ESP.getChipModel());
   logd("Chip Revision: %d", ESP.getChipRevision());
   logd("Number of CPU Cores: %d", ESP.getChipCores());
   logd("CPU Frequency: %d MHz", ESP.getCpuFreqMHz());
   logd("Flash Memory Size: %d MB", ESP.getFlashChipSize() / (1024 * 1024));
   logd("Flash Frequency: %d MHz", ESP.getFlashChipSpeed() / 1000000);
   logd("Heap Size: %d KB", ESP.getHeapSize() / 1024);
   logd("Free Heap: %d KB", ESP.getFreeHeap() / 1024);
   logd("------------ESP32 specifications ---------------");

   Wire.begin(I2C_SDA, I2C_SCL);
   if (!ads.begin(0x48, &Wire)) {
      loge("Failed to initialize ADS.");
   }


   _plc.setup();
   // Configure main worker thread
   _workerThread1->onRun([]() { _plc.CleanUp(); });
   _workerThread1->setInterval(5000);
   _controller.add(_workerThread1);
   _workerThread2->onRun([]() { _plc.Monitor(); });
   _workerThread2->setInterval(200);
   _controller.add(_workerThread2);
   _workerThread3->onRun([]() { _plc.Process(); });
   _workerThread3->setInterval(200);
   _controller.add(_workerThread3);
   esp_task_wdt_init(60, true); // 60-second timeout, panic on timeout
   esp_task_wdt_add(NULL);
   logd("Setup Done");
   return ret;
}

void Main::loop() {
   _controller.run();
   esp_task_wdt_reset(); // Feed the watchdog
   delay(10);
}

extern "C" void app_main(void) {
   logi("Creating default event loop");
   // Initialize esp_netif and default event loop
   ESP_ERROR_CHECK(esp_netif_init());
   ESP_ERROR_CHECK(esp_event_loop_create_default());
   logi("Initialising NVS");
   ESP_ERROR_CHECK(nvs_flash_init());
   logi("Calling my_main.setup()");
   ESP_ERROR_CHECK(my_main.setup());
   while (true) {
      my_main.loop();
   }
}
