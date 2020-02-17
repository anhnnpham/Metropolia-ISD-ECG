
#include "ui.h"

#include <Arduino.h>

#ifdef HAVE_SSD1306
#include <Adafruit_SSD1306.h>
#endif

#include "ecg_isd_config.h"

UIScreen::UIScreen(std::string title) : _title(title) {}

UIScreen::~UIScreen() {}

void UIScreen::set_title(std::string title) {
	_title = title;
}

const std::string& UIScreen::title() {
	return _title;
}

void UIScreen::on_enter() {}

void UIScreen::on_leave() {}

class UIMenuScreen : public UIScreen {
	std::vector<std::pair<std::string, std::function<void(UI&)>>> _choices;
	unsigned _index = 0;

public:
	UIMenuScreen(
		std::string title,
		std::vector<std::pair<std::string, std::function<void(UI&)>>> choices)
		: UIScreen(title), _choices(std::move(choices)) {}

	bool on_event(UIEvent event, UI& ui) override {
		switch (event) {
		case UIEvent::UpClicked:
			if (_index <= 0) {
				_index = _choices.size() - 1;
			} else {
				_index--;
			}
			return true;
		case UIEvent::DownClicked:
			if (_index >= _choices.size() - 1) {
				_index = 0;
			} else {
				_index++;
			}
			return true;
		case UIEvent::OkClicked:
			_choices.at(_index).second(ui);
			return true;
		default:
			return false;
		}
	}
	void draw(Adafruit_GFX& gfx) override {
		int i = 0;
		for (const auto& choice : _choices) {
			if (i++ == _index) {
				gfx.print("> ");
			} else {
				gfx.print("- ");
			}
			gfx.println(choice.first.data());
		}
	}
};

class UIWiFiApScreen : public UIScreen {
	std::shared_ptr<SetupWiFi> _setup_wifi;
	int _offset = 0;

public:
	UIWiFiApScreen() : UIScreen("WiFi Access Point") {}

	void set_setup_wifi(std::shared_ptr<SetupWiFi> setup_wifi) {
		_setup_wifi = std::move(setup_wifi);
	}

	bool on_event(UIEvent event, UI& ui) override {
		switch (event) {
		case UIEvent::UpClicked:
			if (_offset <= 0) {
				_offset = 2;
			} else {
				_offset--;
			}
			return true;
		case UIEvent::DownClicked:
			if (_offset >= 2) {
				_offset = 0;
			} else {
				_offset++;
			}
			return true;
		case UIEvent::OkClicked:
			if (_setup_wifi) {
				_setup_wifi->set_ap_enabled(!_setup_wifi->is_ap_enabled());
			}
			return true;
		default:
			return false;
		}
	}

	void draw(Adafruit_GFX& gfx) override {
		if (!_setup_wifi) {
			gfx.println("UI has no connection to WiFi");
			return;
		}

		if (_offset <= 0) {
			gfx.print("State: ");
			if (_setup_wifi->is_ap_enabled()) {
				gfx.println("Enabled");
			} else {
				gfx.println("Disabled");
			}
		}
		if (_offset <= 1) {
			gfx.println("Name:");
		}
		gfx.printf(" %s\n", _setup_wifi->get_ap_name());
		gfx.printf("Password:\n %s\n", _setup_wifi->get_ap_password());
		if (_offset >= 1) {
			gfx.println("Address:");
		}
		if (_offset >= 2) {
			gfx.print(" ");
			gfx.println(_setup_wifi->get_ap_ip_address());
		}
	}
};

UI::UI(SPIClass& spi, SemaphoreHandle_t& spi_mutex)
	: _spi(spi), _spi_mutex(spi_mutex)
#ifdef ARDUINO_NodeMCU_32S
	  ,
	  _up_button(BTN_UP), _down_button(BTN_DOWN), _left_button(BTN_LEFT),
	  _right_button(BTN_RIGHT)
#endif
{
#ifdef HAVE_SSD1306
	auto ssd1306 = std::make_unique<Adafruit_SSD1306>(
		128, 64, &_spi, OLED_SA0, OLED_RST, OLED_CS);

	if (xSemaphoreTake(_spi_mutex, portMAX_DELAY) == pdTRUE) {
		if (ssd1306->begin(SSD1306_SWITCHCAPVCC)) {
			ssd1306->clearDisplay();
			ssd1306->display();
			ssd1306->setTextSize(1);
			ssd1306->setTextColor(SSD1306_WHITE);
			ssd1306->cp437(true);
			_gfx = std::move(ssd1306);
		} else {
			Serial.println("SSD1306 init failed");
		}

		xSemaphoreGive(_spi_mutex);
	}
#endif

	std::vector<std::pair<std::string, std::function<void(UI&)>>> choices;
	choices.push_back({
		"WiFi Access Point",
		[=](UI& ui) { ui.push(_wifi_menu); },
	});
	choices.push_back({
		"Measurement",
		[=](UI& ui) { ui.push(_measurement_menu); },
	});
	_main_menu =
		std::make_shared<UIMenuScreen>(std::string("ECG ISD ESP32"), choices);

	_wifi_menu = std::make_shared<UIWiFiApScreen>();

	choices.clear();
	choices.push_back({
		"Start",
		[=](UI& ui) { Serial.println("Start Measurement"); },
	});
	choices.push_back({
		"Stop",
		[=](UI& ui) { Serial.println("Stop Measurement"); },
	});
	_measurement_menu =
		std::make_shared<UIMenuScreen>(std::string("Measurement"), choices);

	reset(_main_menu);
}

UI::~UI() {}

void UI::set_setup_wifi(std::shared_ptr<SetupWiFi> setup_wifi) {
	_setup_wifi = std::move(setup_wifi);

	if (auto wifi_menu = std::static_pointer_cast<UIWiFiApScreen>(_wifi_menu)) {
		wifi_menu->set_setup_wifi(_setup_wifi);
	}
}

void UI::pop(unsigned num) {
	for (int i = 0; i < _min((unsigned long) num, _stack.size() - 1); i++) {
		_stack.back()->on_leave();
		_stack.pop_back();
	}
}

void UI::push(std::shared_ptr<UIScreen> screen) {
	_stack.back()->on_leave();
	_stack.push_back(screen);
	_stack.back()->on_enter();
}

void UI::replace(std::shared_ptr<UIScreen> screen) {
	if (_stack.size() == 0) {
		push(screen);
	} else {
		_stack.back()->on_leave();
		_stack.back() = screen;
		_stack.back()->on_enter();
	}
}

void UI::reset(std::shared_ptr<UIScreen> screen) {
	for (int i = 0; i < _stack.size(); i++) {
		_stack.back()->on_leave();
		_stack.pop_back();
	}
	_stack.push_back(screen);
	_stack.back()->on_enter();
}

void UI::loop() {
	while (true) {
#ifdef ARDUINO_NodeMCU_32S
		_up_button.update();
		_down_button.update();
		_left_button.update();
		_right_button.update();
#endif

		UIEvent event = UIEvent::None;

#ifdef ARDUINO_NodeMCU_32S
		if (_up_button.clicked()) {
			event = UIEvent::UpClicked;
		}

		if (_down_button.clicked()) {
			event = UIEvent::DownClicked;
		}

		if (_left_button.clicked()) {
			event = UIEvent::BackClicked;
		}

		if (_right_button.clicked()) {
			event = UIEvent::OkClicked;
		}
#endif

		if (event != UIEvent::None) {
			if (!_stack.empty()) {
				if (!_stack.back()->on_event(event, *this)) {
					switch (event) {
					case UIEvent::BackClicked:
						pop();
						break;
					default:
						break;
					}
				}

				_dirty = true;
			}
		}

		if (_gfx && _dirty && xSemaphoreTake(_spi_mutex, 0) == pdTRUE) {
#ifdef HAVE_SSD1306
			static_cast<Adafruit_SSD1306&>(*_gfx).clearDisplay();
#else
			_gfx->fillScreen(0);
#endif
			_gfx->setCursor(0, 10);
			if (!_stack.empty()) {
				_stack.back()->draw(*_gfx);
			} else {
				_gfx->println("No UIScreen");
			}

			// Header
			_gfx->setCursor(1, 0);
			if (_stack.size() > 1) {
				_gfx->print("< ");
			}
			if (!_stack.empty()) {
				_gfx->println(_stack.back()->title().data());
			} else {
				_gfx->println("!! ERROR !!");
			}
			_gfx->drawFastHLine(0, 8, 128, 1);

			// Footer
			int16_t footer_top = 64 - 7 - 2;
			_gfx->drawFastHLine(0, footer_top, 128, 1);
			_gfx->setCursor(1, footer_top + 2);
			if (_stack.size() > 1) {
				_gfx->print("Back");
			}
			_gfx->setCursor(128 - 6 * 2, footer_top + 2);
			_gfx->print("OK");
#ifdef HAVE_SSD1306
			static_cast<Adafruit_SSD1306&>(*_gfx).display();
#endif
			xSemaphoreGive(_spi_mutex);
			_dirty = false;
		}

		delay(1000 / 60 * portTICK_PERIOD_MS);
	}
}
