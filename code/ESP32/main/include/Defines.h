
#pragma once

#include "GPIO_pins.h"

#define TAG "CLASSICDIY"

#define WATCHDOG_TIMEOUT 10 // time in seconds to trigger the watchdog reset
#define STR_LEN 64
#define EEPROM_SIZE 1024
#define AP_BLINK_RATE 600
#define NC_BLINK_RATE 100
// #define AP_TIMEOUT 1000
#define AP_TIMEOUT 30000
#define FLASHER_TIMEOUT 10000
#define WS_CLIENT_CLEANUP 5000
#define WIFI_CONNECTION_TIMEOUT 120000
#define DEFAULT_AP_PASSWORD "12345678"

#define ADC_Resolution 65536.0
#define SAMPLESIZE 5
#define MQTT_PUBLISH_RATE_LIMIT 500 // delay between MQTT publishes

#define ASYNC_WEBSERVER_PORT 80
#define DNS_PORT 53

#define INPUT_REGISTER_BASE_ADDRESS 1000
#define COIL_BASE_ADDRESS 2000
#define DISCRETE_BASE_ADDRESS 3000
#define HOLDING_REGISTER_BASE_ADDRESS 4000

#ifdef Has_OLED_Display
// OLED display definitions
#define SCREEN_WIDTH 128 // OLED display width, in pixels 
#define SCREEN_HEIGHT 64 // OLED display height, in pixels 
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#endif

