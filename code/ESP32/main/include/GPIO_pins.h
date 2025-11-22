#pragma once
#include <Arduino.h>

#ifdef EDGEBOX
// Modbus setup
#define CoilsDiv ""
#define InputRegistersDiv ""
#define DiscretesDiv ""
#define HoldingRegistersDiv ""

#define DI_PINS 4          // Number of digital input pins
#define DO_PINS 6          // Number of digital output pins
#define AI_PINS 4          // Number of analog input pins
#define AO_PINS 2          // Number of analog output pins
#ifndef LOG_TO_SERIAL_PORT // disable logs to use LED wifi status
// use LED if the log level is none (edgeBox shares the LED pin with the serial TX gpio)
#define WIFI_STATUS_PIN 43 // LED Pin
#endif
#define FACTORY_RESET_PIN 2 // Clear NVRAM, shared with CAN_RXD

void inline GPIO_Init() {}

// Programming and Debugging Port
#define U0_TXD GPIO_NUM_43
#define U0_RXD GPIO_NUM_44

// I2C
#define I2C_SDA GPIO_NUM_20
#define I2C_SCL GPIO_NUM_19

// I2C INT fro RTC PCF8563
#define I2C_INT GPIO_NUM_9

// SPI BUS for W5500 Ethernet Port Driver
#define ETH_SS GPIO_NUM_10
#define ETH_MOSI GPIO_NUM_12
#define ETH_MISO GPIO_NUM_11
#define ETH_SCK GPIO_NUM_13
#define ETH_INT GPIO_NUM_14
#define ETH_RST GPIO_NUM_15

// A7670G
#define LTE_AIRPLANE_MODE GPIO_NUM_16
#define LTE_PWR_EN GPIO_NUM_21
#define LTE_TXD GPIO_NUM_48
#define LTE_RXD GPIO_NUM_47

// RS485
#define RS485_TXD GPIO_NUM_17
#define RS485_RXD GPIO_NUM_18
#define RS485_RTS GPIO_NUM_8

// CAN BUS
#define CAN_TXD GPIO_NUM_1
#define CAN_RXD GPIO_NUM_2

// BUZZER
#define BUZZER GPIO_NUM_45

#define DO0 GPIO_NUM_40
#define DO1 GPIO_NUM_39
#define DO2 GPIO_NUM_38
#define DO3 GPIO_NUM_37
#define DO4 GPIO_NUM_36
#define DO5 GPIO_NUM_35

#define DI0 GPIO_NUM_4
#define DI1 GPIO_NUM_5
#define DI2 GPIO_NUM_6
#define DI3 GPIO_NUM_7

// Analog Input (SGM58031) channels
#define AI0 GPIO_NUM_0
#define AI1 GPIO_NUM_1
#define AI2 GPIO_NUM_2
#define AI3 GPIO_NUM_3

// Analog Output
#define AO0 GPIO_NUM_42
#define AO1 GPIO_NUM_41

#endif
#ifdef NORVI_GSM_AE02

// Modbus setup
#define CoilsDiv ""
#define InputRegistersDiv ""
#define DiscretesDiv ""
#define HoldingRegistersDiv "class=\"hidden\""

#define DI_PINS 8        // Number of digital input pins
#define DO_PINS 2        // Number of digital output pins
#define AI_PINS 4        // Number of analog input pins
#define AO_PINS 0        // Number of analog output pins

#define BUTTONS GPIO_NUM_36 // Analog pin to read buttons

void inline GPIO_Init() { pinMode(GPIO_NUM_36, INPUT); }

// Programming and Debugging Port
#define U0_TXD GPIO_NUM_03
#define U0_RXD GPIO_NUM_01

// I2C
#define I2C_SDA GPIO_NUM_16
#define I2C_SCL GPIO_NUM_17

// GSM Modem
#define LTE_PWR_EN GPIO_NUM_21
#define LTE_TXD GPIO_NUM_32
#define LTE_RXD GPIO_NUM_33

// RS485
#define RS485_TXD GPIO_NUM_26
#define RS485_RXD GPIO_NUM_25
#define RS485_RTS GPIO_NUM_22

#define DO0 GPIO_NUM_12
#define DO1 GPIO_NUM_2

#define DI0 GPIO_NUM_27
#define DI1 GPIO_NUM_34
#define DI2 GPIO_NUM_35
#define DI3 GPIO_NUM_14
#define DI4 GPIO_NUM_13
#define DI5 GPIO_NUM_5
#define DI6 GPIO_NUM_15
#define DI7 GPIO_NUM_19

// Analog Input (ADS1115) channels
#define AI0 GPIO_NUM_0
#define AI1 GPIO_NUM_1
#define AI2 GPIO_NUM_2
#define AI3 GPIO_NUM_3

// No Analog output

// OLED display definitions
#define SCREEN_ADDRESS 0x3C // OLED 128X64 I2C address
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1    // Reset pin # (or -1 if sharing Arduino reset pin)

#endif
#ifdef LILYGO_T_SIM7600G

// Modbus setup
#define CoilsDiv ""
#define InputRegistersDiv "class=\"hidden\""
#define DiscretesDiv ""
#define HoldingRegistersDiv "class=\"hidden\""

#define DI_PINS 2 // Number of digital input pins
#define DO_PINS 2 // Number of digital output pins
#define AI_PINS 0 // Number of analog input pins
#define AO_PINS 0 // Number of analog output pins

#define WIFI_STATUS_PIN 12  // LED Pin
#define FACTORY_RESET_PIN 2 // Clear NVRAM

void inline GPIO_Init() {}

// Programming and Debugging Port
#define U0_TXD GPIO_NUM_01
#define U0_RXD GPIO_NUM_03

// I2C
#define I2C_SDA GPIO_NUM_21
#define I2C_SCL GPIO_NUM_22

// GSM Modem
#define LTE_AIRPLANE_MODE  25 // SIM7600G airplane mode pin, High to exit.
#define LTE_PWR_EN GPIO_NUM_4         // send power to the modem
#define LTE_TXD GPIO_NUM_27
#define LTE_RXD GPIO_NUM_26

// digital outputs
#define DO0 GPIO_NUM_14
#define DO1 GPIO_NUM_15

// digital inputs
#define DI0 GPIO_NUM_12
#define DI1 GPIO_NUM_13

// No Analog output

#endif
#ifdef Waveshare_Relay_6CH

// Modbus setup
#define CoilsDiv ""
#define InputRegistersDiv "class=\"hidden\""
#define DiscretesDiv "class=\"hidden\""
#define HoldingRegistersDiv "class=\"hidden\""

#define DI_PINS 0 // Number of digital input pins
#define DO_PINS 6 // Number of digital output pins
#define AI_PINS 0 // Number of analog input pins
#define AO_PINS 0 // Number of analog output pins

#define RGB_LED_PIN 38
#define GPIO_PIN_Buzzer 21   // Buzzer Control GPIO
#define FACTORY_RESET_PIN 12 // Clear NVRAM
#define PWM_Channel 1        // PWM Channel
#define Frequency 1000       // PWM frequencyconst
#define Resolution 8
#define Dutyfactor 200

void inline RGB_Light(uint8_t red_val, uint8_t green_val, uint8_t blue_val) {
   neopixelWrite(RGB_LED_PIN, green_val, red_val, blue_val); // RGB color adjustment
}

void inline Buzzer_PWM(uint16_t Time) // ledChannel：PWM Channe    dutyfactor:dutyfactor
{
   ledcWrite(PWM_Channel, Dutyfactor);
   delay(Time);
   ledcWrite(PWM_Channel, 0);
}

void inline GPIO_Init() {
   pinMode(RGB_LED_PIN, OUTPUT);     // Initialize the control GPIO of RGB
   pinMode(GPIO_PIN_Buzzer, OUTPUT); // Initialize the control GPIO of Buzzer

   ledcSetup(PWM_Channel, Frequency, Resolution); // Set PWM channel
   ledcAttachPin(GPIO_PIN_Buzzer, PWM_Channel);   // Connect the channel to the corresponding pin
}
// UARTS
#define U0_TXD GPIO_NUM_43
#define U0_RXD GPIO_NUM_44

// RS485
#define RS485_TXD GPIO_NUM_17
#define RS485_RXD GPIO_NUM_18
#define RS485_RTS -1

// I2C
#define I2C_SDA GPIO_NUM_47
#define I2C_SCL GPIO_NUM_48

// digital outputs
#define DO0 GPIO_NUM_1
#define DO1 GPIO_NUM_2
#define DO2 GPIO_NUM_41
#define DO3 GPIO_NUM_42
#define DO4 GPIO_NUM_45
#define DO5 GPIO_NUM_46

// analog outputs
#define AO0 GPIO_NUM_7
#define AO1 GPIO_NUM_8
#define AO2 GPIO_NUM_9
#define AO3 GPIO_NUM_10

#endif
#ifdef Lilygo_Relay_4CH

// Modbus setup
#define CoilsDiv ""
#define InputRegistersDiv "class=\"hidden\""
#define DiscretesDiv "class=\"hidden\""
#define HoldingRegistersDiv "class=\"hidden\""

#define DI_PINS 0 // Number of digital input pins
#define DO_PINS 4 // Number of digital output pins
#define AI_PINS 0 // Number of analog input pins
#define AO_PINS 0 // Number of analog output pins

#define FACTORY_RESET_PIN 4 // Clear NVRAM
#define WIFI_STATUS_PIN 25  // LED Pin

// I2C
#define I2C_SDA GPIO_NUM_15
#define I2C_SCL GPIO_NUM_14

// OLED display definitions
#define SCREEN_ADDRESS 0x3C // OLED 128X64 I2C address
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1    // Reset pin # (or -1 if sharing Arduino reset pin)

// digital outputs
#define DO0 GPIO_NUM_21
#define DO1 GPIO_NUM_19
#define DO2 GPIO_NUM_18
#define DO3 GPIO_NUM_5

#endif
#ifdef ESP_32Dev

// Modbus setup
#define CoilsDiv ""
#define InputRegistersDiv "class=\"hidden\""
#define DiscretesDiv ""
#define HoldingRegistersDiv "class=\"hidden\""

#define DI_PINS 2 // Number of digital input pins
#define DO_PINS 2 // Number of digital output pins
#define AI_PINS 0 // Number of analog input pins
#define AO_PINS 0 // Number of analog output pins

#define WIFI_STATUS_PIN 2  // LED Pin
#define FACTORY_RESET_PIN 4 // Clear NVRAM

// I2C
#define I2C_SDA GPIO_NUM_21
#define I2C_SCL GPIO_NUM_22

// digital outputs
#define DO0 GPIO_NUM_36
#define DO1 GPIO_NUM_39

// digital inputs
#define DI0 GPIO_NUM_34
#define DI1 GPIO_NUM_35

#endif
