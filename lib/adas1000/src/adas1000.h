
#ifndef ADAS1000_H
#define ADAS1000_H

#include <Arduino.h>
#include <SPI.h>

#include "adas1000_reg.h"

#define ADAS1000_READ 0x00000000
#define ADAS1000_WRITE 0x80000000

enum class Adas1000Error {
	None,
};

enum class Adas1000FrameRate {
	Freq_31_25_Hz,
	Freq_2_kHz,
	Freq_16_kHz,
	Freq_128_kHz,
};

struct Adas1000Frame {
public:
	uint16_t updated_mask;
	float ecg[5];
	uint32_t pace;
	uint32_t respiration_magnitude;
	uint32_t respiration_phase;
	uint8_t lead_off_mask;
	uint32_t gpio;
	uint32_t crc;
};

namespace Adas1000FrameField {
	constexpr std::uint16_t Lead1 = (1 << 15);
	constexpr std::uint16_t Lead2 = (1 << 14);
	constexpr std::uint16_t Lead3 = (1 << 13);
	constexpr std::uint16_t V1 = (1 << 12);
	constexpr std::uint16_t V2 = (1 << 11);

	constexpr std::uint16_t LeftArm = Lead1;
	constexpr std::uint16_t LeftLeg = Lead2;
	constexpr std::uint16_t RightArm = Lead3;

	constexpr std::uint16_t PaceDetection = (1 << 5);
	constexpr std::uint16_t RespirationMagnitude = (1 << 4);
	constexpr std::uint16_t RespirationPhase = (1 << 3);
	constexpr std::uint16_t LeadOffStatus = (1 << 2);
	constexpr std::uint16_t GPIO = (1 << 1);
	constexpr std::uint16_t CRC = (1 << 0);
}

class Adas1000 {
	Adas1000Error _error = Adas1000Error::None;
	SPIClass& _spi;
	int _cs_pin;

	Adas1000FrameRate _frame_rate = Adas1000FrameRate::Freq_31_25_Hz;
	uint32_t _frame_fields;
	int _num_frame_fields;
	float _ecg_multiplier;
	uint32_t _spi_clock_rate_min;
	uint32_t _spi_clock_rate_max;

	inline void _set_cs() {
		digitalWrite(_cs_pin, LOW);
	}

	inline void _clr_cs() {
		digitalWrite(_cs_pin, HIGH);
	}

	void _update_clock_rates();
	void _update_frmctl();

public:
	Adas1000(SPIClass& spi, int cs_pin);

	inline Adas1000Error error() {
		return _error;
	}

	std::uint32_t read_register(std::uint8_t address);
	void write_register(std::uint8_t address, std::uint32_t);

	void do_software_reset();

	unsigned spi_clock_rate_min() {
		return _spi_clock_rate_min;
	}

	unsigned spi_clock_rate_max() {
		return _spi_clock_rate_max;
	}

	void set_frame_rate(Adas1000FrameRate frame_rate);
	void set_frame_fields(std::uint32_t fields);

	void start_reading();
	int read_frame(Adas1000Frame& frame);
};

#endif
