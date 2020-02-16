#ifndef ECG_ISD_ESP32_UI_H
#define ECG_ISD_ESP32_UI_H

class SPIClass;
typedef void* SemaphoreHandle_t;

class UI {
	SPIClass& _spi;
	SemaphoreHandle_t _spi_mutex;

public:
	UI(SPIClass& spi, SemaphoreHandle_t& spi_mutex);
	~UI();
	void loop();
};

#endif
