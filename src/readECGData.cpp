#include "readECGData.h"

#include <memory>

#include <Arduino.h>
#include <SPI.h>

#include "ecg_isd_config.h"

enum class ReadECGDataMessage : uint8_t {
	Read,
	StartRecording,
	StopRecording,
};

ReadECGData::ReadECGData(SPIClass& spi)
	: _spi(spi), _msg_queue(xQueueCreate(5, sizeof(ReadECGDataMessage))) {}

ReadECGData::~ReadECGData() {}

void ReadECGData::set_ring_buffer(RingbufHandle_t ring_buffer) {
	_ring_buf = ring_buffer;
}

bool ReadECGData::start_measurement() {
	log_i("sending start msg");
	ReadECGDataMessage msg = ReadECGDataMessage::StartRecording;
	if (xQueueSendToBack(_msg_queue, &msg, 0) == pdTRUE) {
		return true;
	}

	log_e("failed to send start msg");

	return false;
}

bool ReadECGData::stop_measurement() {
	log_i("sending stop msg");
	ReadECGDataMessage msg = ReadECGDataMessage::StopRecording;
	if (xQueueSendToBack(_msg_queue, &msg, 0) == pdTRUE) {
		return true;
	}

	log_e("failed to send stop msg");

	return false;
}

// void ReadECGData::on_drdy(ReadECGData* self) {
// 	ReadECGDataMessage msg = ReadECGDataMessage::Read;
// 	BaseType_t higherPriorityTaskAwoken = pdFALSE;
// 	xQueueSendToFrontFromISR(self->_msg_queue, &msg, &higherPriorityTaskAwoken);
// 	if (higherPriorityTaskAwoken == pdTRUE) {
// 		portYIELD_FROM_ISR();
// 	}
// }

void ReadECGData::loop() {
	if (_msg_queue == 0) {
		log_e("Couldn't create message queue");
		while (true) {
		}
	}

	log_i("Hello from ReadECGData");

	gpio_set_drive_capability((gpio_num_t) ADAS1000_SCK, GPIO_DRIVE_CAP_0);
	gpio_set_drive_capability((gpio_num_t) ADAS1000_SDO, GPIO_DRIVE_CAP_0);
	gpio_set_drive_capability((gpio_num_t) ADAS1000_SDI, GPIO_DRIVE_CAP_0);
	gpio_set_drive_capability((gpio_num_t) ADAS1000_nCS_0, GPIO_DRIVE_CAP_0);
	gpio_set_drive_capability((gpio_num_t) ADAS1000_nCS_1, GPIO_DRIVE_CAP_0);
	_spi.begin(ADAS1000_SCK, ADAS1000_SDO, ADAS1000_SDI);
	_spi.setBitOrder(SPI_MSBFIRST);
	_spi.setDataMode(SPI_MODE3);
	_spi.setFrequency(2048 * 13 * 32 * 1);

	_ecg0 = std::make_unique<Adas1000>(_spi, ADAS1000_nCS_0);
	_ecg1 = std::make_unique<Adas1000>(_spi, ADAS1000_nCS_1);

	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, LOW);

	log_i("Setting up ADAS1000");

	_ecg0->do_software_reset();

	namespace Field = Adas1000FrameField;
	_ecg0->set_frame_fields(
		// clang-format off
		Field::Lead1 | Field::Lead2 | Field::Lead3 | Field::V1 | Field::V2
		// |
		// Field::PaceDetection |
		// Field::RespirationMagnitude | Field::RespirationPhase | Field::CRC |
		// Field::GPIO | Field::LeadOffStatus
		// clang-format on
	);

	_ecg0->set_frame_rate(Adas1000FrameRate::Freq_2_kHz);
	_ecg1->set_frame_rate(Adas1000FrameRate::Freq_2_kHz);
	unsigned spi_frequency =
		_ecg0->spi_clock_rate_min() + _ecg1->spi_clock_rate_min();
	// multiplier for making sure other things slowing things down doesn't make
	// us drop frames
	// spi_frequency *= 2;
	spi_frequency = min(spi_frequency, _ecg0->spi_clock_rate_max());
	spi_frequency = min(spi_frequency, _ecg1->spi_clock_rate_max());
	_spi.setFrequency(spi_frequency);
	log_i("Serial clock rate: %u", spi_frequency);

	_ecg0->write_register(
		ADAS1000_CMREFCTL,
		// clang-format off
		0
		| ADAS1000_CMREFCTL_LACM
		| ADAS1000_CMREFCTL_LLCM
		| ADAS1000_CMREFCTL_RACM
		| ADAS1000_CMREFCTL_DRVCM
		| ADAS1000_CMREFCTL_RLDEN
		| ADAS1000_CMREFCTL_SHDLEN
		// clang-format on
	);

	_ecg0->write_register(
		ADAS1000_ECGCTL,
		// clang-format off
		0
		| ADAS1000_ECGCTL_LAEN_ENABLED
		| ADAS1000_ECGCTL_LLEN_ENABLED
		| ADAS1000_ECGCTL_RAEN_ENABLED
		| ADAS1000_ECGCTL_V1EN_DISABLED
		| ADAS1000_ECGCTL_V2EN_DISABLED
		| ADAS1000_ECGCTL_CHCONFIG_DIFFERENTIAL
		| ADAS1000_ECGCTL_GAIN_X1_4
		| ADAS1000_ECGCTL_VREFBUF_ENABLED
		| ADAS1000_ECGCTL_CLKEXT_XTAL
		| ADAS1000_ECGCTL_MASTER_MASTER
		| ADAS1000_ECGCTL_GANG_SINGLE_CHANNEL_MODE
		| ADAS1000_ECGCTL_HP_2_MSPS_HIGH_PERFORMANCE_LOW_NOISE
		| ADAS1000_ECGCTL_CNVEN_CONVERSION_ENABLE
		| ADAS1000_ECGCTL_PWREN_POWER_ENABLE
		| ADAS1000_ECGCTL_SWRST_NOP
		// clang-format on
	);

	// _ecg0->write_register(ADAS1000_CMREFCTL, 0x00000B);
	// _ecg0->write_register(ADAS1000_TESTTONE, 0xF8000D);
	// _ecg0->write_register(ADAS1000_FILTCTL, 0x000008);
	// _ecg0->write_register(ADAS1000_FRMCTL, 0x079610);

	// _ecg0->write_register(ADAS1000_ECGCTL, 0xF800AE);

	log_i("Dumping ecg0 registers:");
	for (int i = 0x00; i <= 0x0F; i++) {
		log_i("@%02x: %08x", i, _ecg0->read_register(i));
	}
	log_i("Dumping ecg1 registers:");
	for (int i = 0x00; i <= 0x0F; i++) {
		log_i("@%02x: %08x", i, _ecg1->read_register(i));
	}
	vTaskDelay(500 * portTICK_PERIOD_MS);

	// pinMode(ADAS1000_nDRDY, INPUT);
	// attachInterruptArg(
	// 	ADAS1000_nDRDY,
	// 	reinterpret_cast<void (*)(void*)>(on_drdy),
	// 	this,
	// 	FALLING);
	// on_drdy(this);

	// _ecg1->start_reading();

	ReadECGDataMessage msg;
	int missed_frames = 0;
	Adas1000Frame ecg0_frame;
	// Adas1000Frame ecg1_frame;

	while (true) {
		if (xQueueReceive(_msg_queue, &msg, 0)) {
			log_i("got msg: %d", msg);

			switch (msg) {
			case ReadECGDataMessage::Read:
				missed_frames = 0;
				missed_frames += _ecg0->read_frame(ecg0_frame);
				// missed_frames += _ecg1->read_frame(ecg1_frame);

				break;
			case ReadECGDataMessage::StartRecording:
				_ecg0->start_reading();
				_state = ReadECGDataState::Measuring;
				break;
			case ReadECGDataMessage::StopRecording:
				_ecg0->write_register(ADAS1000_NOP, ADAS1000_NOP);
				_state = ReadECGDataState::Idle;
				break;
			}

			// Serial.printf(
			// 	"missed: %d, ecg: [%f, %f, %f, %f, %f], pace: %d, resp: (%d, "
			// 	"%d)\n",
			// 	missed_frames,
			// 	ecg0_frame.ecg[0],
			// 	ecg0_frame.ecg[1],
			// 	ecg0_frame.ecg[2],
			// 	ecg0_frame.ecg[3],
			// 	ecg0_frame.ecg[4],
			// 	ecg0_frame.pace,
			// 	ecg0_frame.respiration_magnitude,
			// 	ecg0_frame.respiration_phase);
		} else if (_state == ReadECGDataState::Measuring) {
			missed_frames += _ecg0->read_frame(ecg0_frame);
			// missed_frames += _ecg1->read_frame(ecg1_frame);

			digitalWrite(LED_BUILTIN, missed_frames > 0 ? HIGH : LOW);

			if (_ring_buf &&
				xRingbufferSend(
					_ring_buf, ecg0_frame.ecg, sizeof(ecg0_frame.ecg), 0) ==
					pdTRUE) {
				if (missed_frames > 0) {
					log_w("Missed %d frames", missed_frames);
					missed_frames = 0;
				}
			}

			// Serial.printf(
			// 	"mis: %d, ecg: [%f, %f, %f, %f, %f], pac: %d, res: (%d, %d)\n",
			// 	missed_frames, ecg0_frame.ecg[0], ecg0_frame.ecg[1],
			// 	ecg0_frame.ecg[2], ecg0_frame.ecg[3], ecg0_frame.ecg[4],
			// 	ecg0_frame.pace, ecg0_frame.respiration_magnitude,
			// 	ecg0_frame.respiration_phase);

			// Serial.printf(
			// 	"%d,%f,%f,%f,%f,%f\n",
			// 	missed_frames,
			// 	ecg0_frame.ecg[0],
			// 	ecg0_frame.ecg[1],
			// 	ecg0_frame.ecg[2],
			// 	ecg0_frame.ecg[3],
			// 	ecg0_frame.ecg[4]);
		}
	}
}
