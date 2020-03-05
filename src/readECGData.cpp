#include "readECGData.h"

#include <Arduino.h>

ReadECGData::ReadECGData(SPIClass& spi) : _spi(spi) {}

ReadECGData::~ReadECGData() {}

void ReadECGData::loop() {
	while (true) {
		delay(1000);
	}
}
