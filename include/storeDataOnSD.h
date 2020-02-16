#ifndef ECG_ISD_ESP32_STOREDATAONSD_H
#define ECG_ISD_ESP32_STOREDATAONSD_H

#include <memory>

class Storage;

class StoreDataOnSD {
	std::shared_ptr<Storage> _storage;

public:
	StoreDataOnSD(std::shared_ptr<Storage> storage);
	~StoreDataOnSD();
	void loop();
};

#endif
