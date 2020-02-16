#ifndef ECG_ISD_ESP32_READECGDATA_H
#define ECG_ISD_ESP32_READECGDATA_H

class SPIClass;

class ReadECGData {
	SPIClass& _spi;

public:
	ReadECGData(SPIClass& spi);
	~ReadECGData();
	void loop();
};

#endif
