#include <Arduino.h>
#include <freertos/FreeRTOS.h>

#include "setupWiFi.h"
#include "ui.h"

void uiTask(void* parameter);

SetupWiFi* setupWiFi;
UI* ui;

void setup() {
	Serial.begin(921600);

	setupWiFi = new SetupWiFi;
	ui = new UI;

	// Give Time to Complete Wifi
	delay(2000);

	xTaskCreate(uiTask, "UI", 5000, nullptr, 1, nullptr);
}

void loop() {
	//Just do nothing
	delay(1000);
}

void uiTask(void* parameter) {
	ui->loop();
}
