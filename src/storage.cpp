#include "storage.h"

#include <Arduino.h>

Storage::Storage(SPIClass& spi, SemaphoreHandle_t& spi_mutex)
	: _spi(spi), _spi_mutex(spi_mutex) {}

Storage::~Storage() {}
