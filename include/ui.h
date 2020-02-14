#ifndef ECG_ISD_ESP32_UI_H
#define ECG_ISD_ESP32_UI_H

#include <memory>

#include <Adafruit_GFX.h>

#ifdef ARDUINO_NodeMCU_32S
#include <SimpleButton.h>
#endif

class SPIClass;
typedef void* SemaphoreHandle_t;

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
	SemaphoreHandle_t _spi_mutex;

	std::shared_ptr<Adafruit_GFX> _gfx;
	std::vector<std::shared_ptr<UIScreen>> _stack;
	bool _dirty = true;

#ifdef ARDUINO_NodeMCU_32S
	simplebutton::Button _up_button;
	simplebutton::Button _down_button;
	simplebutton::Button _left_button;
	simplebutton::Button _right_button;
#endif

	std::shared_ptr<UIScreen> _main_menu;
	std::shared_ptr<UIScreen> _wifi_menu;
	std::shared_ptr<UIScreen> _measurement_menu;

public:
	UI(SPIClass& spi, SemaphoreHandle_t& spi_mutex);
	~UI();

	void pop(unsigned num = 1);
	void push(std::shared_ptr<UIScreen> screen);
	void replace(std::shared_ptr<UIScreen> screen);
	void reset(std::shared_ptr<UIScreen> screen);

	void loop();
};

#endif
