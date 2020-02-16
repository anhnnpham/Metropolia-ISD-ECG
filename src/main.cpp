#include <Arduino.h>
#include <SPI.h>
#include <freertos/FreeRTOS.h>

#include "readECGData.h"
#include "setupWiFi.h"
#include "storage.h"
#include "storeDataOnSD.h"
#include "ui.h"

SPIClass vspi(VSPI);  // For ECG
SPIClass hspi(HSPI);  // For OLED and SD
SemaphoreHandle_t hspiMutex;

void readECGDataTask(void* parameter);
void storeDataOnSDTask(void* parameter);
void uiTask(void* parameter);

std::shared_ptr<ReadECGData> readECGData;
std::shared_ptr<SetupWiFi> setupWiFi;
std::shared_ptr<Storage> storage;
std::shared_ptr<StoreDataOnSD> storeDataOnSD;
std::unique_ptr<UI> ui;

void setup() {
	Serial.begin(921600);

	delay(2000);

	Serial.println("Starting");

	hspiMutex = xSemaphoreCreateMutex();

	readECGData = std::make_shared<ReadECGData>(vspi);
	setupWiFi = std::make_shared<SetupWiFi>();
	storage = std::make_shared<Storage>(hspi, hspiMutex);
	storeDataOnSD = std::make_shared<StoreDataOnSD>(storage);
	ui = std::make_unique<UI>(hspi, hspiMutex);

	xTaskCreate(readECGDataTask, "ReadECGData", 5000, nullptr, 1, nullptr);
	xTaskCreate(storeDataOnSDTask, "StoreDataOnSD", 5000, nullptr, 1, nullptr);
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
