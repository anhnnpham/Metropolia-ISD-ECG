#ifndef ECG_ISD_ESP32_READECGDATA_H
#define ECG_ISD_ESP32_READECGDATA_H

class SPIClass; // sys lib

class ReadECGData {
	SPIClass& _spi; // ref

public:
	ReadECGData(SPIClass& spi); // ReadECGData::ReadECGData(SPIClass& spi) : _spi(spi)
	~ReadECGData();
	void loop();
};

#endif
