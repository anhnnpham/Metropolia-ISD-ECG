#ifndef ECG_ISD_ESP32_UI_H
#define ECG_ISD_ESP32_UI_H

#include <memory>
#include <mutex>

#include <Adafruit_GFX.h>

#ifdef ARDUINO_NodeMCU_32S
#include <SimpleButton.h>
#endif

#include "setupWiFi.h"

class SPIClass; // sys lib

class UI;

enum class UIEvent {
	None,
	UpClicked,
	DownClicked,
	BackClicked,
	OkClicked,
};

class UIScreen {
	std::string _title;

protected:
	void set_title(std::string title);

public:
	UIScreen() = delete;
	UIScreen(std::string title);
	virtual ~UIScreen();

	const std::string& title();

	virtual void on_enter();
	virtual void on_leave();

	virtual bool on_event(UIEvent event, UI& ui) = 0;
	virtual void draw(Adafruit_GFX& gfx) = 0;
};

class UI {
	SPIClass& _spi;
	std::mutex& _spi_mutex;

	std::unique_ptr<Adafruit_GFX> _gfx;
	std::vector<std::shared_ptr<UIScreen>> _stack;
	bool _dirty = true;

#ifdef ARDUINO_NodeMCU_32S
	simplebutton::Button _up_button;
	simplebutton::Button _down_button;
	simplebutton::Button _left_button;
	simplebutton::Button _right_button;
#endif

	std::shared_ptr<UIScreen> _main_menu; // obj
	std::shared_ptr<UIScreen> _wifi_menu;
	std::shared_ptr<UIScreen> _measurement_menu;

	std::shared_ptr<SetupWiFi> _setup_wifi;

public:
	UI(SPIClass& spi, std::mutex& spi_mutex);
	~UI();

	void set_setup_wifi(std::shared_ptr<SetupWiFi> setup_wifi);

	void pop(unsigned num = 1);
	void push(std::shared_ptr<UIScreen> screen);
	void replace(std::shared_ptr<UIScreen> screen);
	void reset(std::shared_ptr<UIScreen> screen);

	void loop();
};

#endif
