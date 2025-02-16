
#pragma once

#define TAG "ESP_PLC"

#define STR_LEN 255  // general string buffer size
#define CONFIG_LEN 32 // configuration string buffer size
#define NUMBER_CONFIG_LEN 6
#define LEVEL_CONFIG_LEN 2
#define AP_TIMEOUT 30000
#define DEFAULT_AP_PASSWORD "12345678"

#define CheckBit(var,pos) ((var) & (1<<(pos))) ? true : false
#define toShort(i, v) (v[i++]<<8) | v[i++]

#define ADC_Resolution 4095.0
#define SAMPLESIZE 20
#define MQTT_PUBLISH_RATE_LIMIT 500 // delay between MQTT publishes

#define ASYNC_WEBSERVER_PORT 80
#define IOTCONFIG_PORT 7667
#define WSOCKET_LOG_PORT 7668
#define WSOCKET_HOME_PORT 7669

#define DI_PINS 12	// Number of digital input pins
#define DO_PINS 4	// Number of digital output pins
#define AI_PINS 4	// Number of analog input pins
