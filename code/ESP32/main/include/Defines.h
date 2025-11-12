
#pragma once

#include "GPIO_pins.h"

#define TAG "ESP_PLC"

#define WATCHDOG_TIMEOUT 10 // time in seconds to trigger the watchdog reset
#define STR_LEN 64
#define EEPROM_SIZE 2048
#define AP_BLINK_RATE 600
#define NC_BLINK_RATE 100

#define AP_TIMEOUT 1000
// #define AP_TIMEOUT 30000 //set back to 1000 in production
#define FLASHER_TIMEOUT 10000
#define WS_CLIENT_CLEANUP 5000
#define WIFI_CONNECTION_TIMEOUT 120000
#define DEFAULT_AP_PASSWORD "12345678"

#define ADC_Resolution 65536.0
#define SAMPLESIZE 5
#define MQTT_PUBLISH_RATE_LIMIT 500 // delay between MQTT publishes

#define MODBUS_POLL_RATE 1000
#define MODBUS_RTU_TIMEOUT 2000
#define MODBUS_RTU_REQUEST_QUEUE_SIZE 64

#define ASYNC_WEBSERVER_PORT 80
#define DNS_PORT 53

#define INPUT_REGISTER_BASE_ADDRESS 1000
#define COIL_BASE_ADDRESS 2000
#define DISCRETE_BASE_ADDRESS 3000
#define HOLDING_REGISTER_BASE_ADDRESS 4000
