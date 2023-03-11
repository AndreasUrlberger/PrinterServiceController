#include "DisplayController.h"

DisplayController::DisplayController(){
	const char* filename = "/dev/i2c-3";
	oled = ssd1306_i2c_open(filename, 0x3c, displayWidth, displayHeight, NULL);
	if (!oled) {
		throw std::runtime_error("Could not create the screen");
	}
	if (ssd1306_i2c_display_initialize(oled) < 0) {
		ssd1306_i2c_close(oled);
		throw std::runtime_error("Failed to initialize the display. Check if it is connected !");
	}
	fbp = ssd1306_framebuffer_create(oled->width, oled->height, oled->err);
	ssd1306_i2c_display_clear(oled);	
	turnOn();

	opts[0].type = ssd1306_graphics_options_t::SSD1306_OPT_FONT_FILE;
	opts[0].value.font_file = "/usr/share/fonts/truetype/freefont/FreeMono.ttf";
	opts[1].type = ssd1306_graphics_options_t::SSD1306_OPT_ROTATE_FONT;
	opts[1].value.rotation_degrees = 180;
}

DisplayController::~DisplayController()
{
	if (oled != nullptr && fbp != nullptr) {
		ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_POWER_OFF, 0, 0);
		ssd1306_framebuffer_destroy(fbp);
		fbp = nullptr;
		ssd1306_i2c_close(oled);
		oled = nullptr;
	}
}

void DisplayController::turnOff()
{
	if (isOn) {
		ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_POWER_OFF, 0, 0);
		isOn = false;
	}
}

void DisplayController::turnOn()
{
	if (!isOn) {
		setInverted(!isInverted);
		ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_POWER_ON, 0, 0);
		isOn = true;
	}
}

bool DisplayController::isIconVisible()
{
	return iconVisible;
}

void DisplayController::setIconVisible(bool visible)
{
	iconVisible = visible;
}

void DisplayController::setInverted(bool inverted)
{
	isInverted = inverted;
	if (isInverted) {
		ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_DISP_INVERTED, 0, 0);
	}
	else {
		ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_DISP_NORMAL, 0, 0);
	}
}

void DisplayController::drawFanIcon(uint8_t xOff, uint8_t yOff) {
	for (uint8_t x = 0; x < 17; ++x) {
		for (uint8_t y = 0; y < 17; y++)
		{
			uint8_t xCord = static_cast<uint8_t>(x + xOff);
			uint8_t yCord = static_cast<uint8_t>(y + yOff);
			//ssd1306_framebuffer_put_pixel(fbp, xCord, yCord, fan[x][y]); // add rotation
			ssd1306_framebuffer_put_pixel_rotation(fbp, xCord, yCord, fan[x][y], 180);
		}
	}
}

void DisplayController::drawTemperature(int32_t want, int32_t have, std::string name)
{
	ssd1306_framebuffer_clear(fbp);
	int haveInt = have / 1000;
	int haveDec = std::abs(have % 1000) / 100; // one decimal digit
	std::string firstLine = std::to_string(haveInt) + "." + std::to_string(haveDec) + "/" + std::to_string(want/1000);
	// draw have integer
	ssd1306_framebuffer_draw_text_extra(fbp, firstLine.c_str(), 0, 127, 37, SSD1306_FONT_CUSTOM, fontSize, opts, 2, &bbox);
	// rest of first line
	ssd1306_framebuffer_draw_text_extra(fbp, "�", 0, 34, 50, SSD1306_FONT_CUSTOM, fontSize, opts, 2, &bbox);
	ssd1306_framebuffer_draw_text_extra(fbp, "C", 0, 24, 37, SSD1306_FONT_CUSTOM, fontSize, opts, 2, &bbox);

	// draw PTEG
	ssd1306_framebuffer_draw_text_extra(fbp, name.c_str(), 0, 127, 5, SSD1306_FONT_CUSTOM, fontSize, opts, 2, &bbox);
	// draw fan icon
	if (iconVisible) {
		drawFanIcon(5, 4);
	}

	ssd1306_i2c_display_update(oled, fbp);
}