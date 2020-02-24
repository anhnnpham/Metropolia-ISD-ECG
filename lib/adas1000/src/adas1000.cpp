
#include "adas1000.h"

Adas1000::Adas1000(SPIClass& spi, int cs_pin) : _spi(spi), _cs_pin(cs_pin) {
	pinMode(_cs_pin, OUTPUT);
	digitalWrite(_cs_pin, HIGH);
	_update_clock_rates();
}

std::uint32_t Adas1000::read_register(std::uint8_t address) {
	_set_cs();
	_spi.write32(ADAS1000_READ | (address & 0x7F) << 24);
	auto value = _spi.transfer32(0);
	_clr_cs();
	return value;
}

void Adas1000::write_register(std::uint8_t address, std::uint32_t value) {
	_set_cs();
	_spi.write32(
		ADAS1000_WRITE | (address & 0x7F) << 24 | (value & 0x00FFFFFF));
	_clr_cs();
}

void Adas1000::do_software_reset() {
	write_register(ADAS1000_ECGCTL, ADAS1000_ECGCTL_SWRST_RESET);
	write_register(ADAS1000_NOP, 0);
	delay(3);
}

void Adas1000::_update_clock_rates() {
	float frame_rate = 0;
	int bits_per_word = 0;
	switch (_frame_rate) {
	case Adas1000FrameRate::Freq_31_25_Hz:
		frame_rate = 31.25;
		bits_per_word = 32;
		break;
	case Adas1000FrameRate::Freq_2_kHz:
		frame_rate = 2000;
		bits_per_word = 32;
		break;
	case Adas1000FrameRate::Freq_16_kHz:
		frame_rate = 16'000;
		bits_per_word = 32;
		break;
	case Adas1000FrameRate::Freq_128_kHz:
		frame_rate = 128'000;
		bits_per_word = 16;
		break;
	}

	// number of data fields + one for busy header + one for actual header
	int words_per_frame = _num_frame_fields + 2;

	bool is_high_performance = false;

	_spi_clock_rate_min = frame_rate * words_per_frame * bits_per_word;
	_spi_clock_rate_max =
		min((1.024e6 * (1 + is_high_performance) * words_per_frame *
			 bits_per_word) /
				3,
			40e6);
}

void Adas1000::_update_frmctl() {
	int num_fields = 0;
	std::uint32_t mask = 0b000001111000000000000000;

#define DO_FIELD(FIELD, MASK) \
	if (_frame_fields & (FIELD)) { \
		num_fields++; \
		mask |= (MASK##_INCLUDE); \
	} else { \
		mask |= (MASK##_EXCLUDE); \
	}

	namespace Field = Adas1000FrameField;

	DO_FIELD(Field::Lead1, ADAS1000_FRMCTL_LEAD1_LADIS);
	DO_FIELD(Field::Lead2, ADAS1000_FRMCTL_LEAD2_LLDIS);
	DO_FIELD(Field::Lead3, ADAS1000_FRMCTL_LEAD3_RADIS);
	DO_FIELD(Field::V1, ADAS1000_FRMCTL_V1DIS);
	DO_FIELD(Field::V2, ADAS1000_FRMCTL_V2DIS);
	DO_FIELD(Field::PaceDetection, ADAS1000_FRMCTL_PACEDIS);
	DO_FIELD(Field::RespirationMagnitude, ADAS1000_FRMCTL_RESPMDIS);
	DO_FIELD(Field::RespirationPhase, ADAS1000_FRMCTL_RESPPHDIS);
	DO_FIELD(Field::LeadOffStatus, ADAS1000_FRMCTL_LOFFDIS);
	DO_FIELD(Field::GPIO, ADAS1000_FRMCTL_GPIODIS);
	DO_FIELD(Field::CRC, ADAS1000_FRMCTL_CRCDIS);

#undef DO_FIELD

	mask |= ADAS1000_FRMCTL_ADIS_FIXED_FRAME_FORMAT;
	mask |= ADAS1000_FRMCTL_RDYRPT_REPEAT_FRAME_HEADER_UNTIL_READY;
	mask |= ADAS1000_FRMCTL_DATAFMT_DIGITAL_LEAD_VECTOR_FORMAT;
	mask |= ADAS1000_FRMCTL_SKIP_OUTPUT_EVERY_OTHER_FRAME;

	switch (_frame_rate) {
	case Adas1000FrameRate::Freq_31_25_Hz:
		mask |= ADAS1000_FRMCTL_FRMRATE_31_25_HZ;
		break;
	case Adas1000FrameRate::Freq_2_kHz:
		mask |= ADAS1000_FRMCTL_FRMRATE_2_KHZ;
		break;
	case Adas1000FrameRate::Freq_16_kHz:
		mask |= ADAS1000_FRMCTL_FRMRATE_16_KHZ;
		break;
	case Adas1000FrameRate::Freq_128_kHz:
		mask |= ADAS1000_FRMCTL_FRMRATE_128_KHZ;
		break;
	}

	write_register(ADAS1000_FRMCTL, mask);
	_num_frame_fields = num_fields;
}

void Adas1000::set_frame_rate(Adas1000FrameRate frame_rate) {
	_frame_rate = frame_rate;
	_update_frmctl();
	_update_clock_rates();
}

void Adas1000::set_frame_fields(std::uint32_t fields) {
	_frame_fields = fields;
	_update_frmctl();
	_update_clock_rates();
}

void Adas1000::start_reading() {
	_set_cs();
	_spi.write32(0x40000000);
	_clr_cs();
}

int Adas1000::read_frame(Adas1000Frame& frame) {

	_set_cs();

#define IS_HEADER(WORD) ((WORD & 0x8000'0000) == 0x8000'0000)
#define IS_HEADER_BUSY(WORD) ((WORD & 0xC000'0000) == 0xC000'0000)

	uint32_t word = 0;

	do {
		word = _spi.transfer32(0);
		// Serial.printf("W 0x%08x\n", word);
	} while (!IS_HEADER(word));

	while (IS_HEADER_BUSY(word)) {
		word = _spi.transfer32(0);
		// Serial.printf("B 0x%08x\n", word);
	}

	// 	Serial.printf("H 0x%08x\n", word);

	if (!IS_HEADER(word)) {
		log_e("header not found");
		return -1;
	}

#undef IS_HEADER
#undef IS_HEADER_BUSY

#define CHECK_FIELD(WORD, MASK, VARIANT) ((WORD & MASK) == MASK##_##VARIANT)

	if (CHECK_FIELD(word, ADAS1000_FRAMES_FAULT, ERROR_CONDITION)) {
		log_e("read fault");
		return -2;
	}

	int dropped_frames = (word >> 28) & 0x0000'0003;

	int num_words = _num_frame_fields;

#if 0
	// PACEDIS, RESPMDIS, LOFFDIS, may be excluded
	if ((word & ADAS1000_FRAMES_PACE_3_DETECTED) == 0) {
		num_words--;
	}
	if ((word & ADAS1000_FRAMES_PACE_2_DETECTED) == 0) {
		num_words--;
	}
	if ((word & ADAS1000_FRAMES_PACE_1_DETECTED) == 0) {
		num_words--;
	}
	if ((word & ADAS1000_FRAMES_RESPIRATION) == 0) {
		num_words--;
	}
	if ((word & ADAS1000_FRAMES_LEAD_OFF_DETECTED) == 0) {
		num_words--;
	}
#endif

	int num_bits = 0;

	switch (_frame_rate) {
	case Adas1000FrameRate::Freq_31_25_Hz:
	case Adas1000FrameRate::Freq_2_kHz:
	case Adas1000FrameRate::Freq_16_kHz:
		num_bits = 24;
		break;
	case Adas1000FrameRate::Freq_128_kHz:
		num_bits = 16;
		break;
	}

	float vref = 1.8;
	float gain = 1.4;
	_ecg_multiplier = (4 * vref / gain) / (pow(2, num_bits) - 1);

	frame.updated_mask = 0;

	namespace Field = Adas1000FrameField;

	auto calculate_ecg = [=](uint32_t value) {
		if (value & 0x0080'0000) {
			return -(((value ^ 0x00FFFFFF) + 1) * _ecg_multiplier);
		} else {
			return value * _ecg_multiplier;
		}
	};

	for (int i = 0; i < num_words; i++) {
		word = _spi.transfer32(0);
		// Serial.printf("D 0x%08x\n", word);

		switch ((word & 0xFF00'0000) >> 24) {
		case 0x06:  // GPIO
			frame.updated_mask |= Field::GPIO;
			frame.gpio = word & 0x00FFFFFF;
			break;
		case 0x11:  // LEAD I/LA
			frame.updated_mask |= Field::Lead1;
			frame.ecg[0] = calculate_ecg(word & 0x00FFFFFF);
			break;
		case 0x12:  // LEAD II/LL
			frame.updated_mask |= Field::Lead2;
			frame.ecg[1] = calculate_ecg(word & 0x00FFFFFF);
			break;
		case 0x13:  // LEAD III/RA
			frame.updated_mask |= Field::Lead3;
			frame.ecg[2] = calculate_ecg(word & 0x00FFFFFF);
			break;
		case 0x14:  // LEAD V1'/V1
			frame.updated_mask |= Field::V1;
			frame.ecg[3] = calculate_ecg(word & 0x00FFFFFF);
			break;
		case 0x15:  // LEAD V2'/V2
			frame.updated_mask |= Field::V2;
			frame.ecg[4] = calculate_ecg(word & 0x00FFFFFF);
			break;
		case 0x1A:  // PACE
			frame.updated_mask |= Field::PaceDetection;
			frame.pace = word & 0x00FFFFFF;
			break;
		case 0x1B:  // RESPM
			frame.updated_mask |= Field::RespirationMagnitude;
			frame.respiration_magnitude = word & 0x00FFFFFF;
			break;
		case 0x1C:  // RESPPH
			frame.updated_mask |= Field::RespirationPhase;
			frame.respiration_phase = word & 0x00FFFFFF;
			break;
		case 0x1D:  // LOFF
			frame.updated_mask |= Field::LeadOffStatus;
			frame.lead_off_mask = word & 0x00FFFFFF;
			break;
		case 0x41:  // CRC
			frame.updated_mask |= Field::CRC;
			frame.crc = word & 0x00FFFFFF;
			break;
		default:
			log_w("Unhandled field: %08x", word);
			break;
		}
	}

	_clr_cs();

	return dropped_frames;
}
