#ifndef ECG_ISD_ESP32_READECGDATA_H
#define ECG_ISD_ESP32_READECGDATA_H

#include <memory>

#include <adas1000.h>
#include <freertos/FreeRTOS.h>
#include <freertos/ringbuf.h>

class SPIClass;

enum class ReadECGDataState {
	Idle,
	Measuring,
};

class ReadECGData {
	SPIClass& _spi;
	QueueHandle_t _msg_queue;
	RingbufHandle_t _ring_buf;
	ReadECGDataState _state = ReadECGDataState::Idle;

	std::unique_ptr<Adas1000> _ecg0;
	std::unique_ptr<Adas1000> _ecg1;

	// static void on_drdy(ReadECGData* self);

public:
	ReadECGData(SPIClass& spi);
	~ReadECGData();

	void set_ring_buffer(RingbufHandle_t ring_buffer);

	bool start_measurement();
	bool is_measuring();
	bool stop_measurement();

	void loop();
};

#endif
