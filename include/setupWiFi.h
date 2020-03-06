#ifndef ECG_ISD_ESP32_SETUPWIFI_H
#define ECG_ISD_ESP32_SETUPWIFI_H

#include <WiFi.h>

class SetupWiFi {
	const char* _ssid = "ESP32-Access-Point";
	const char* _password = "123456789";

public:
	SetupWiFi();
	~SetupWiFi();

	const char* get_ap_name(); // getter
	const char* get_ap_password();
	IPAddress get_ap_ip_address();

	bool set_ap_enabled(bool enabled = true);
	bool is_ap_enabled();
};

#endif
