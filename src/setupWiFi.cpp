#include "setupWiFi.h"

SetupWiFi::SetupWiFi() {
}

SetupWiFi::~SetupWiFi() {
	turnOff();
}

IPAddress SetupWiFi::getIP() {
	return WiFi.softAPIP();
}

bool SetupWiFi::turnOff() {
	return WiFi.softAPdisconnect(true);
}

bool SetupWiFi::turnOn() {
	return WiFi.softAP(ssid, password);
}
