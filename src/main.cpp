#include <mutex>

#include <Arduino.h>
#include <SPI.h>
#include <freertos/FreeRTOS.h>

#include "readECGData.h"
#include "setupWiFi.h"
#include "storage.h"
#include "storeDataOnSD.h"
#include "webAccess.h"
#include "ui.h"

SPIClass vspi(VSPI);  // For ECG
SPIClass hspi(HSPI);  // For OLED and SD
std::mutex hspi_mutex;

void readECGDataTask(void* parameter);
void storeDataOnSDTask(void* parameter);
void uiTask(void* parameter);
void webAccessTask(void* parameter);

std::shared_ptr<ReadECGData> readECGData; // obj
std::shared_ptr<SetupWiFi> setupWiFi;
std::shared_ptr<Storage> storage;
std::shared_ptr<StoreDataOnSD> storeDataOnSD;
std::unique_ptr<WebAccess> webAccess;
std::unique_ptr<UI> ui;

void setup() {
	Serial.begin(921600);

	delay(2000);

	Serial.println("Starting");

	readECGData = std::make_shared<ReadECGData>(vspi);
	setupWiFi = std::make_shared<SetupWiFi>();
	setupWiFi->set_ap_enabled(true);
	storage = std::make_shared<Storage>(hspi, hspi_mutex);
	storeDataOnSD = std::make_shared<StoreDataOnSD>(storage);
	webAccess = std::make_unique<WebAccess>(storage);
	ui = std::make_unique<UI>(hspi, hspi_mutex);
	ui->set_setup_wifi(setupWiFi);

	xTaskCreate(readECGDataTask, "ReadECGData", 5000, nullptr, 1, nullptr);
	xTaskCreate(storeDataOnSDTask, "StoreDataOnSD", 5000, nullptr, 1, nullptr);
	xTaskCreate(webAccessTask, "WebAccess", 5000, nullptr, 1, nullptr);
	xTaskCreate(uiTask, "UI", 5000, nullptr, 1, nullptr);
}

void loop() {
	// Just do nothing
	delay(1000);
}

void readECGDataTask(void* parameter) {
	readECGData->loop();
}

void storeDataOnSDTask(void* parameter) {
	storeDataOnSD->loop();
}

void uiTask(void* parameter) {
	ui->loop();
}

void webAccessTask(void* parameter) {
	webAccess->loop();
}
