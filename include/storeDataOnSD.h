#ifndef ECG_ISD_ESP32_STOREDATAONSD_H
#define ECG_ISD_ESP32_STOREDATAONSD_H

#include <atomic>
#include <memory>
#include <mutex>

#include <freertos/FreeRTOS.h>
#include <freertos/ringbuf.h>

class Storage;
class ReadECGData;

class StoreDataOnSD {
	std::shared_ptr<Storage> _storage;
	mutable std::mutex _storage_mutex;
	std::shared_ptr<ReadECGData> _readECGData;
	RingbufHandle_t _ring_buffer;
	std::atomic_bool _is_recording;

public:
	StoreDataOnSD(
		std::shared_ptr<Storage> storage,
		std::shared_ptr<ReadECGData> readECGData);
	~StoreDataOnSD();

	const char* start_recording();
	bool is_recording() const;
	void stop_recording();

	void loop();
};

#endif
