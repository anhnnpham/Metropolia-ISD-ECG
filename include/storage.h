#ifndef ECG_ISD_ESP32_STORAGE_H
#define ECG_ISD_ESP32_STORAGE_H

class SPIClass;
typedef void* SemaphoreHandle_t;

class Storage {
	SPIClass& _spi;
	SemaphoreHandle_t _spi_mutex;

public:
	Storage(SPIClass& spi, SemaphoreHandle_t& spi_mutex);
	~Storage();
};

#endif
