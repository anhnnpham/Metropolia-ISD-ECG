#ifndef ECG_ISD_ESP32_STORAGE_H
#define ECG_ISD_ESP32_STORAGE_H

#include <SD.h>

class SPIClass;
typedef void* SemaphoreHandle_t;

enum class StorageError {
	None,
	CannotInitialize,
	FileSystemError,
	TooManyFiles,
	CanNotOpenFile
};

enum class StorageState { Idle, Error, Recording };

class Storage {
	SPIClass& _spi;
	SemaphoreHandle_t _spi_mutex;

	int lastFileIndex = 0;
	File currentFile;
	StorageState state_ = StorageState::Idle;
	StorageError error_ = StorageError::None;

	bool init();
	void setError(StorageError error);

public:
	Storage(SPIClass& spi, SemaphoreHandle_t spi_mutex);
	~Storage();

	bool beginRecording();
	bool endRecording();
	bool writeRecord(const float data[], uint8_t length);
	int readRecord(float data[], uint8_t length);
	void loop();
	StorageError getError() const;
	bool clearError();
	StorageState isRecording();
};

#endif
