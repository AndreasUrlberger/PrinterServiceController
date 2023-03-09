#pragma once 

#include <ssd1306_i2c.h>
#include <stdexcept>

class DisplayController {

private: 
	static constexpr bool fan[17][17] = {
		{false,	false,	false,	false,	false,	false,	false,	true,	true,	true,	true,	false,	false,	false,	false,	false,	false,},
		{false,	false,	false,	false,	false,	false,	true,	true,	true,	true,	true,	true,	false,	false,	false,	false,	false,},
		{false,	false,	false,	false,	false,	false,	true,	true,	true,	true,	true,	true,	false,	false,	false,	false,	false,},
		{false,	false,	false,	false,	false,	false,	true,	true,	true,	true,	true,	true,	false,	false,	false,	false,	false,},
		{false,	false,	false,	false,	false,	false,	false,	true,	true,	true,	true,	false,	false,	false,	false,	false,	false,},
		{false,	true,	true,	true,	false,	false,	false,	true,	true,	false,	false,	false,	false,	false,	false,	false,	false,},
		{true,	true,	true,	true,	true,	false,	false,	true,	true,	true,	false,	false,	true,	true,	true,	true,	false,},
		{true,	true,	true,	true,	true,	true,	true,	true,	false,	true,	true,	true,	true,	true,	true,	true,	true,},
		{true,	true,	true,	true,	true,	true,	true,	false,	true,	false,	true,	true,	true,	true,	true,	true,	true,},
		{true,	true,	true,	true,	true,	true,	true,	true,	false,	true,	true,	true,	true,	true,	true,	true,	true,},
		{false,	true,	true,	true,	true,	false,	false,	true,	true,	true,	false,	false,	true,	true,	true,	true,	true,},
		{false,	false,	false,	false,	false,	false,	false,	true,	true,	true,	false,	false,	false,	true,	true,	true,	false,},
		{false,	false,	false,	false,	false,	false,	true,	true,	true,	true,	true,	false,	false,	false,	false,	false,	false,},
		{false,	false,	false,	false,	false,	true,	true,	true,	true,	true,	true,	false,	false,	false,	false,	false,	false,},
		{false,	false,	false,	false,	false,	true,	true,	true,	true,	true,	true,	false,	false,	false,	false,	false,	false,},
		{false,	false,	false,	false,	false,	true,	true,	true,	true,	true,	true,	false,	false,	false,	false,	false,	false,},
		{false,	false,	false,	false,	false,	false,	true,	true,	true,	true,	false,	false,	false,	false,	false,	false,	false,},
	};

	bool iconVisible = false;
	uint8_t fontSize = 7;
	ssd1306_framebuffer_t* fbp;
	ssd1306_framebuffer_box_t bbox;
	ssd1306_graphics_options_t opts[2];
	ssd1306_i2c_t* oled;
	bool isOn = false;
	bool isInverted = false;

	void drawFanIcon(uint8_t xOff, uint8_t yOff);

public:
	void drawTemperature(int32_t want, int32_t have, std::string name);

	DisplayController();

	~DisplayController();

	void turnOff();

	void turnOn();

	bool isIconVisible();

	void setIconVisible(bool visible);

	void setInverted(bool inverted);
};