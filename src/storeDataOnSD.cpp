#include "storeDataOnSD.h"

#include <Arduino.h>

#include "readECGData.h"
#include "storage.h"

StoreDataOnSD::StoreDataOnSD(
	std::shared_ptr<Storage> storage, std::shared_ptr<ReadECGData> readECGData)
	: _storage(storage), _readECGData(readECGData),
	  _ring_buffer(xRingbufferCreate(1 << 10, RINGBUF_TYPE_NOSPLIT)) {
	_readECGData->set_ring_buffer(_ring_buffer);
}

StoreDataOnSD::~StoreDataOnSD() {}

const char* StoreDataOnSD::start_recording() {
	std::lock_guard<std::mutex> lock(_storage_mutex);

	const char* recording_name;

	if ((recording_name = _storage->create_new_recording())) {
		if (_readECGData->start_measurement()) {
			log_i("Started recording: %s", recording_name);

			_is_recording.store(true, std::memory_order_relaxed);
			return recording_name;
		} else {
			_storage->close_recording();
			log_e("Failed to start measurement");
		}
	} else {
		log_e("Failed to create new recording");
	}

	return nullptr;
}

bool StoreDataOnSD::is_recording() const {
	return _is_recording.load(std::memory_order_relaxed);
}

void StoreDataOnSD::stop_recording() {
	std::lock_guard<std::mutex> lock(_storage_mutex);

	_readECGData->stop_measurement();
	_storage->close_recording();

	_is_recording.store(false, std::memory_order_relaxed);
}

void StoreDataOnSD::loop() {
	size_t buf_size;
	float* floats;

	while (true) {
		floats =
			static_cast<float*>(xRingbufferReceive(_ring_buffer, &buf_size, 0));

		if (floats) {
			std::lock_guard<std::mutex> lock(_storage_mutex);

			if (is_recording()) {
				if (!_storage->write_record(floats, buf_size / sizeof(float))) {
					log_e("can't write data to storage");
				}
			}

			vRingbufferReturnItem(_ring_buffer, floats);
		} else {
			delay(1);
		}
	}
}
