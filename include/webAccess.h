#ifndef ECG_ISD_ESP32_WEBACCESS_H
#define ECG_ISD_ESP32_WEBACCESS_H

#include "storage.h"
#include <WebServer.h>
// WebServer server(80);

class WebAccess {
	WebServer _server/* (80) */;
	std::shared_ptr<Storage> _storage;

public:
	WebAccess(std::shared_ptr<Storage> storage);
	~WebAccess();
	void handleRoot(); 
	void handleRecordingCsv();
	void handleNotFound();
	void loop();
};

#endif // ECG_ISD_ESP32_WEBACCESS_H
