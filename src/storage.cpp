#include "storage.h"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>

#include "ecg_isd_config.h"

const char* storage_error_to_str(StorageError error) {
	switch (error) {
	case StorageError::None:
		return "None";
	case StorageError::CanNotInitialize:
		return "CanNotInitialize";
	case StorageError::CanNotOpenFile:
		return "CanNotOpenFile";
	case StorageError::CanNotRemoveFile:
		return "CanNotRemoveFile";
	case StorageError::FileSystemError:
		return "FileSystemError";
	case StorageError::TooManyFiles:
		return "TooManyFiles";
	}

	return "<Error>";
}

const char* storage_state_to_str(StorageState state) {
	switch (state) {
	case StorageState::Idle:
		return "Idle";
	case StorageState::Error:
		return "Error";
	case StorageState::Recording:
		return "Recording";
	case StorageState::Reading:
		return "Reading";
	}

	return "<State>";
}

#define STORAGE_CHECK_STATE(CURRENT_STATE, EXPECTED_STATE, RETURN_VALUE) \
	if (_state != (EXPECTED_STATE)) { \
		log_e( \
			"ERROR: not in %s state, current state: %s", \
			storage_state_to_str(EXPECTED_STATE), \
			storage_state_to_str(_state)); \
		return RETURN_VALUE; \
	}

Storage::Storage(SPIClass& spi, SemaphoreHandle_t spi_mutex)
	: _spi(spi), _spi_mutex(spi_mutex) {
	if (!init()) {
		log_e("First init failed");
	}

	// float data[9] = { 0.0, 1.0, 2.0 };
	// if (begin_recording()) {
	// 	write_record(data, 3);
	// 	write_record(data, 3);
	// 	write_record(data, 3);
	// 	end_recording();
	// }
	//
	// log_v("List recs:");
	// for (auto& rec : list_recordings()) {
	// 	log_v("- %s (%d)", rec.get_name(), rec.get_size());
	// 	remove_recording(rec.get_name());
	// 	if (open_recording(rec.get_name())) {
	// 		int len;
	// 		do {
	// 			len = read_record(data, 9);
	//
	// 			for (int i = 0; i < len; i++) {
	// 				Serial.printf("%f;", data[i]);
	// 			}
	//
	// 			if (len > 0) {
	// 				Serial.println();
	// 			}
	// 		} while (len >= 1);
	// 		close_recording();
	// 	}
	// }
}

Storage::~Storage() {}

bool Storage::init() {
	if (xSemaphoreTake(_spi_mutex, portMAX_DELAY) == pdTRUE) {
		if (!SD.begin(SD_CS, _spi)) {
			log_e("begin error");
			set_error(StorageError::CanNotInitialize);

			xSemaphoreGive(_spi_mutex);
			return false;
		}

		if (!SD.exists("/recordings")) {
			if (!SD.mkdir("/recordings")) {
				log_e("mkdir /recordings error");
				set_error(StorageError::FileSystemError);

				xSemaphoreGive(_spi_mutex);
				return false;
			}
		}

		_state = StorageState::Idle;

		xSemaphoreGive(_spi_mutex);
		return true;
	} else {
		log_e("Failed to initialize storage: can not take mutex");
		set_error(StorageError::CanNotInitialize);

		return false;
	}
}

void Storage::set_error(StorageError error) {
	log_e("%s", storage_error_to_str(error));
	_state = StorageState::Error;
	_error = error;
}

StorageError Storage::get_error() const {
	return _error;
}

bool Storage::clear_error() {
	if (!init()) {
		log_e("Clear error failed");
		return false;
	}
	_state = StorageState::Idle;
	_error = StorageError::None;

	return true;
}

bool Storage::begin_recording() {
	STORAGE_CHECK_STATE(_state, StorageState::Idle, false);

	char recording_name[6];
	char recording_path[22];
	bool new_file_found = false;

	if (xSemaphoreTake(_spi_mutex, portMAX_DELAY) == pdTRUE) {
		for (int i = _last_file_index; i < 10000; i++) {
			snprintf(recording_name, 6, "%05d", i);
			snprintf(recording_path, 22, "/recordings/%s.rec", recording_name);

			if (!SD.exists(recording_path)) {
				_current_recording_name = recording_name;
				new_file_found = true;
				break;
			}
		}

		if (!new_file_found) {
			log_e("can not find free filename");
			set_error(StorageError::TooManyFiles);
			xSemaphoreGive(_spi_mutex);
			return false;
		}

		log_i("opening: %s", recording_path);
		_current_file = SD.open(recording_path, FILE_WRITE);

		if (!_current_file) {
			log_e("can not open file: %s", recording_path);
			set_error(StorageError::CanNotOpenFile);
			xSemaphoreGive(_spi_mutex);
			return false;
		}

		_state = StorageState::Recording;

		xSemaphoreGive(_spi_mutex);
		return true;
	}

	return false;
}

bool Storage::is_recording() const {
	return _state == StorageState::Recording;
}

bool Storage::write_record(const float data[], uint8_t length) {
	STORAGE_CHECK_STATE(_state, StorageState::Recording, false);

	if (xSemaphoreTake(_spi_mutex, portMAX_DELAY) == pdTRUE) {
		if (!_current_file.write(length)) {
			log_e("couldn't write length to file");
			set_error(StorageError::FileSystemError);
			return false;
		}

		if (!_current_file.write(
				(const uint8_t*) data, sizeof(float) * length)) {
			log_e("couldn't write data to file");
			set_error(StorageError::FileSystemError);
			return false;
		}

		xSemaphoreGive(_spi_mutex);
		return true;
	}

	return false;
}

const char* Storage::end_recording() {
	STORAGE_CHECK_STATE(_state, StorageState::Recording, nullptr);

	if (_current_file) {
		log_i("closing file");
		if (xSemaphoreTake(_spi_mutex, portMAX_DELAY) == pdTRUE) {
			_current_file.close();
			xSemaphoreGive(_spi_mutex);
		}
	}

	_state = StorageState::Idle;
	_last_file_index++;

	return _current_recording_name.data();
}

std::vector<StorageEntry> Storage::list_recordings() {
	STORAGE_CHECK_STATE(_state, StorageState::Idle, {});

	if (xSemaphoreTake(_spi_mutex, portMAX_DELAY) == pdTRUE) {
		File dir = SD.open("/recordings");

		if (!dir) {
			log_e("Can not open /recordings dir");
			set_error(StorageError::CanNotOpenFile);
			xSemaphoreGive(_spi_mutex);
			return {};
		}

		File entry = dir.openNextFile();
		std::vector<StorageEntry> recordings;

		while (entry) {
			if (!entry.isDirectory()) {
				const char* entry_name = entry.name();

				if (strncmp(entry_name, "/recordings/", 12) == 0) {
					entry_name += 12;

					int entry_name_len = strlen(entry_name);
					if ((entry_name_len >= 4) &&
						(0 ==
						 strcasecmp(entry_name + entry_name_len - 4, ".rec"))) {
						recordings.emplace_back(StorageEntry(
							std::string(entry_name, entry_name_len - 4),
							entry.size()));
					}
				}
			}

			entry.close();
			entry = dir.openNextFile();
		}

		dir.close();

		xSemaphoreGive(_spi_mutex);
		return recordings;
	}

	return {};
}

static std::string build_recording_path(const char* name) {
	std::string path = "/recordings/";
	path += name;
	path += ".rec";
	return path;
}

bool Storage::remove_recording(const char* name) {
	STORAGE_CHECK_STATE(_state, StorageState::Idle, false);

	auto path = build_recording_path(name);

	if (xSemaphoreTake(_spi_mutex, portMAX_DELAY) == pdTRUE) {
		if (SD.exists(path.data())) {
			if (!SD.remove(path.data())) {
				log_e("can't remove file: %s", path.data());
				set_error(StorageError::CanNotRemoveFile);
				xSemaphoreGive(_spi_mutex);
				return false;
			}

			xSemaphoreGive(_spi_mutex);
			return true;
		}

		xSemaphoreGive(_spi_mutex);
	}

	return false;
}

bool Storage::open_recording(const char* name) {
	STORAGE_CHECK_STATE(_state, StorageState::Idle, false);

	auto path = build_recording_path(name);

	if (xSemaphoreTake(_spi_mutex, portMAX_DELAY) == pdTRUE) {
		if (SD.exists(path.data())) {
			if ((_current_file = SD.open(path.data()))) {
				_state = StorageState::Reading;
				xSemaphoreGive(_spi_mutex);
				return true;
			} else {
				log_e("can not open recording: %s", path.data());
			}
		}

		xSemaphoreGive(_spi_mutex);
	}

	return false;
}

int Storage::read_record(float data[], uint8_t length) {
	STORAGE_CHECK_STATE(_state, StorageState::Reading, 0);

	int data_length;

	if (xSemaphoreTake(_spi_mutex, portMAX_DELAY) == pdTRUE) {
		if ((data_length = _current_file.peek()) == -1) {
			// 			log_d("can't read length, assuming end of file");
			xSemaphoreGive(_spi_mutex);
			return 0;
		}

		// 		log_d("reading %d floats", data_length);

		if (data_length >= length) {
			log_w(
				"not enough space for reading, space: %d, needed: %d",
				length,
				data_length);
			xSemaphoreGive(_spi_mutex);
			return -data_length;
		}

		if ((data_length = _current_file.read()) == -1) {
			log_e("couldn't read data from file");
			set_error(StorageError::FileSystemError);
			xSemaphoreGive(_spi_mutex);
			return 0;
		}

		if (!_current_file.read((uint8_t*) data, sizeof(float) * data_length)) {
			set_error(StorageError::FileSystemError);
			xSemaphoreGive(_spi_mutex);
			return 0;
		}

		xSemaphoreGive(_spi_mutex);
		return data_length;
	}

	return 0;
}

bool Storage::close_recording() {
	STORAGE_CHECK_STATE(_state, StorageState::Reading, false);

	if (xSemaphoreTake(_spi_mutex, portMAX_DELAY) == pdTRUE) {
		_current_file.close();
		_state = StorageState::Idle;

		xSemaphoreGive(_spi_mutex);
		return true;
	}

	return false;
}
