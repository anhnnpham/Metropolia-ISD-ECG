#include "setupWiFi.h"

SetupWiFi::SetupWiFi() {}

SetupWiFi::~SetupWiFi() {
	WiFi.softAPdisconnect(true);
}

const char* SetupWiFi::get_ap_name() {
	return _ssid;
}

const char* SetupWiFi::get_ap_password() {
	return _password;
}

IPAddress SetupWiFi::get_ap_ip_address() {
	return WiFi.softAPIP();
}

bool SetupWiFi::set_ap_enabled(bool enabled) {
	if (enabled) {
		return WiFi.softAP(_ssid, _password);
	} else {
		return WiFi.softAPdisconnect(WiFi.getMode() == WIFI_MODE_AP);
	}
}

bool SetupWiFi::is_ap_enabled() {
	const auto wifi_mode = WiFi.getMode();

	return wifi_mode == WIFI_MODE_AP || wifi_mode == WIFI_MODE_APSTA;
}
