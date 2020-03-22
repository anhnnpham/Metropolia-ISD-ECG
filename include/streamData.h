#ifndef ECG_ISD_ESP32_STREAMDATA_H
#define ECG_ISD_ESP32_STREAMDATA_H

// C headers
extern "C"
{
#include <lwip/sockets.h>
#include <esp_log.h>
#include <string.h>
#include <errno.h>
#include "sdkconfig.h"
}
#define PORT_NUMBER 8001

static char tag[] = "socket_server";

class StreamData {
public:
	~StreamData();
	StreamData();
	void loop();
};

#endif

