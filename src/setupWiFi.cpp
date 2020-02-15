#include "setupWiFi.h"

SetupWiFi::SetupWiFi() {
}

SetupWiFi::~SetupWiFi() {
}

IPAddress SetupWiFi::getIP() {
	return WiFi.softAPIP();
}

bool SetupWiFi::turnOff() {
	WiFi.softAPdisconnect(true);
}

bool SetupWiFi::turnOn() {
	WiFi.softAP(ssid, password);
}
