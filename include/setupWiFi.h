#ifndef ECG_ISD_ESP32_SETUPWIFI_H
#define ECG_ISD_ESP32_SETUPWIFI_H

#include <WiFi.h>

class SetupWiFi {
	const char *ssid = "ESP32-Access-Point";
	const char *password = "123456789";

public:
	SetupWiFi();
	~SetupWiFi();
	IPAddress getIP();
	bool turnOff();
	bool turnOn();
};

#endif
