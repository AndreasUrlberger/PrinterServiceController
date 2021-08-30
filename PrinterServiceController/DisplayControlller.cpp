#include "DisplayController.h"

DisplayController::DisplayController(){
	const char* filename = "/dev/i2c-3";
	oled = ssd1306_i2c_open(filename, 0x3c, 128, 64, NULL);
	if (!oled) {
		throw std::runtime_error("Could not create the screen");
	}
	if (ssd1306_i2c_display_initialize(oled) < 0) {
		ssd1306_i2c_close(oled);
		throw std::runtime_error("Failed to initialize the display. Check if it is connected !");
	}
	fbp = ssd1306_framebuffer_create(oled->width, oled->height, oled->err);
	ssd1306_i2c_display_clear(oled);
	ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_POWER_ON, 0, 0);

	opts[0].type = ssd1306_graphics_options_t::SSD1306_OPT_FONT_FILE;
	opts[0].value.font_file = "/usr/share/fonts/truetype/calibri/calibri-regular.ttf";
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
	ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_POWER_OFF, 0, 0);
}

void DisplayController::turnOn()
{
	ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_POWER_ON, 0, 0);
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
	if (inverted) {
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
			ssd1306_framebuffer_put_pixel(fbp, xCord, yCord, fan[x][y]);
		}
	}
}

void DisplayController::drawTemperature(int32_t want, int32_t have)
{
	ssd1306_framebuffer_clear(fbp);
	int haveInt = have / 1000;
	int haveDec = std::abs(have % 1000) / 100; // one decimal digit

	// draw dot
	ssd1306_framebuffer_draw_text_extra(fbp, ".", 0, 29, 20, SSD1306_FONT_CUSTOM, fontSize, opts, 1, &bbox);
	// draw have integer
	ssd1306_framebuffer_draw_text_extra(fbp, std::to_string(haveInt).c_str(), 0, 0, 5, SSD1306_FONT_CUSTOM, fontSize, opts, 1, &bbox);
	// rest of first line
	std::string firstLine = std::to_string(haveDec) + "/" + std::to_string(want) + "°C";
	ssd1306_framebuffer_draw_text_extra(fbp, firstLine.c_str(), 0, 39, 5, SSD1306_FONT_CUSTOM, fontSize, opts, 1, &bbox);

	// draw PTEG
	ssd1306_framebuffer_draw_text_extra(fbp, "PETG", 0, 0, 32 + 5, SSD1306_FONT_CUSTOM, fontSize, opts, 1, &bbox);
	// draw fan icon
	if (iconVisible) {
		drawFanIcon(101, 32 + 5);
	}

	ssd1306_i2c_display_update(oled, fbp);
}