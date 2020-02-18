#ifndef ECG_ISD_ESP32_STORAGE_H
#define ECG_ISD_ESP32_STORAGE_H

#include <SD.h>

class SPIClass;
typedef void* SemaphoreHandle_t;

enum class StorageError {
	None,
	CanNotInitialize,
	CanNotOpenFile,
	CanNotRemoveFile,
	FileSystemError,
	TooManyFiles,
};

enum class StorageState {
	Idle,
	Error,
	Recording,
	Reading,
};

class StorageEntry {
	std::string _name;
	size_t _size;

	StorageEntry(std::string name, size_t size)
		: _name(std::move(name)), _size(size) {}

public:
	const char* get_name() const {
		return _name.data();
	}

	size_t get_size() const {
		return _size;
	}

	friend class Storage;
};

class Storage {
	SPIClass& _spi;
	SemaphoreHandle_t _spi_mutex;

	int _last_file_index = 0;
	File _current_file;
	std::string _current_recording_name;
	StorageState _state = StorageState::Idle;
	StorageError _error = StorageError::None;

	bool init();
	void set_error(StorageError error);

public:
	Storage(SPIClass& spi, SemaphoreHandle_t spi_mutex);
	~Storage();

	StorageError get_error() const;
	bool clear_error();

	std::vector<StorageEntry> list_recordings();
	bool remove_recording(const char* name);

	const char* create_new_recording();
	bool write_record(const float data[], uint8_t length);

	bool open_recording(const char* name);
	int read_record(float data[], uint8_t length);

	bool is_recording_open() const;
	bool close_recording();
};

#endif
