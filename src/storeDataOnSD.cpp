#include "storeDataOnSD.h"

#include <Arduino.h>

StoreDataOnSD::StoreDataOnSD(std::shared_ptr<Storage> storage)
	: _storage(storage) {}

StoreDataOnSD::~StoreDataOnSD() {}

void StoreDataOnSD::loop() {
	while (true) {
		delay(1000);
	}
}
