#include "DisplayController.hpp"

DisplayController::DisplayController(const char* filename, const uint8_t displayHeight, const uint8_t displayWidth, const uint8_t fontSize, const char* fontName) : filename(filename), displayHeight(displayHeight), displayWidth(displayWidth), fontSize(fontSize), fontName(fontName) {
    // Create dump file. This is done to avoid the display output to be printed to the console.
    auto nullFile = fopen("/dev/null", "w");
    oled = ssd1306_i2c_open(filename, 0x3c, displayWidth, displayHeight, nullFile);
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
    opts[0].value.font_file = fontName;
    opts[1].type = ssd1306_graphics_options_t::SSD1306_OPT_ROTATE_FONT;
    opts[1].value.rotation_degrees = 180;
}

DisplayController::~DisplayController() {
    if (oled != nullptr) {
        ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_POWER_OFF, 0, 0);
        ssd1306_i2c_close(oled);
        oled = nullptr;
    }

    if (fbp != nullptr) {
        ssd1306_framebuffer_destroy(fbp);
        fbp = nullptr;
    }
}

void DisplayController::turnOff() {
    if (isOn) {
        isOn = false;
        ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_POWER_OFF, 0, 0);
    }
}

void DisplayController::turnOn() {
    if (!isOn) {
        isOn = true;
        setInverted(!isInverted);
        ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_POWER_ON, 0, 0);
    }
}

bool DisplayController::isIconVisible() const {
    return iconVisible;
}

void DisplayController::setIconVisible(const bool visible) {
    iconVisible = visible;
}

void DisplayController::setInverted(const bool inverted) {
    isInverted = inverted;
    if (isInverted) {
        ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_DISP_INVERTED, 0, 0);
    } else {
        ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_DISP_NORMAL, 0, 0);
    }
}

void DisplayController::drawFanIcon(const uint8_t xOff, const uint8_t yOff) {
    for (uint8_t x = 0; x < 17; ++x) {
        for (uint8_t y = 0; y < 17; y++) {
            uint8_t xCord = static_cast<uint8_t>(x + xOff);
            uint8_t yCord = static_cast<uint8_t>(y + yOff);
            // Screen is upside down, therefore we need to rotate the icon 180 degrees.
            ssd1306_framebuffer_put_pixel_rotation(fbp, xCord, yCord, fanIcon[x][y], 180);
        }
    }
}

void DisplayController::drawTemperature(const int32_t want, const int32_t have, const std::string name) {
    ssd1306_framebuffer_clear(fbp);
    const int haveInt = have / 1000;
    const int haveDec = std::abs(have % 1000) / 100;  // one decimal digit
    std::string firstLine = std::to_string(haveInt) + "." + std::to_string(haveDec) + "/" + std::to_string(want / 1000);
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