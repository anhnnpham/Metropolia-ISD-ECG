#include "ui.h"

#include <Arduino.h>

UI::UI(SPIClass& spi, SemaphoreHandle_t& spi_mutex)
	: _spi(spi), _spi_mutex(spi_mutex) {
}

UI::~UI() {
}

void UI::loop() {
	while (true) {
		delay(1000);
	}
}
