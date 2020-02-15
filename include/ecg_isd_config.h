
#ifndef ECG_ISD_CONFIG_H
#define ECG_ISD_CONFIG_H

#include <Arduino.h>

#ifdef ARDUINO_NodeMCU_32S

constexpr int8_t ADAS1000_SCK = 18;  // SCK
constexpr int8_t ADAS1000_SDI = 23;  // MOSI
constexpr int8_t ADAS1000_SDO = 19;  // MISO
constexpr int8_t ADAS1000_nCS_0 = 22;
constexpr int8_t ADAS1000_nCS_1 = 21;
constexpr int8_t ADAS1000_nDRDY = 15;

constexpr int8_t BTN_UP = 39;  // VN
constexpr int8_t BTN_LEFT = 34;
constexpr int8_t BTN_RIGHT = 33;
constexpr int8_t BTN_DOWN = 35;

constexpr int8_t OLED_SDA = 13;
constexpr int8_t OLED_SCL = 14;
constexpr int8_t OLED_SA0 = 25;
constexpr int8_t OLED_RST = 26;
constexpr int8_t OLED_CS = 27;

constexpr int8_t SD_CD = 4;
constexpr int8_t SD_DO = 12;
constexpr int8_t SD_DI = 13;
constexpr int8_t SD_SCK = 14;
constexpr int8_t SD_CS = 32;

#endif

#endif
