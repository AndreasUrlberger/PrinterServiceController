#include "LightController.h"

#include <hueplusplus/LinHttpHandler.h>

LightController::LightController() {
}

void LightController::start() {
    auto handler = std::make_shared<hueplusplus::LinHttpHandler>();
    hueplusplus::Bridge bridge(BRIDGE_IP, 80, BRIDGE_USERNAME, handler);
    this->printerLight = std::make_unique<hueplusplus::Light>(bridge.lights().get(PRINTER_LIGHT_ID));
}

// Switch light on and handle all exceptions. Return true if successfull.
bool LightController::switchOn() {
    bool isOn = false;

    try {
        printerLight->on();
        isOn = true;
    } catch (std::system_error &error) {
        std::cerr << "Error while switching on light: " << error.what() << "\n";
    } catch (hueplusplus::HueAPIResponseException &error) {
        std::cerr << "HueAPIResponseException while switching on light: " << error.what() << "\n";
    } catch (hueplusplus::HueException &error) {
        std::cerr << "HueException while switching on light: " << error.what() << "\n";
    } catch (nlohmann::json::parse_error &error) {
        std::cerr << "nlohmann::json::parse_error while switching on light: " << error.what() << "\n";
    } catch (...) {
        std::cerr << "Unknown error while switching on light\n";
    }

    return isOn;
}

// Switch light off and handle all exceptions. Return true if successfull.
bool LightController::switchOff() {
    bool isOff = false;

    try {
        printerLight->off();
        isOff = true;
    } catch (std::system_error &error) {
        std::cerr << "Error while switching off light: " << error.what() << "\n";
    } catch (hueplusplus::HueAPIResponseException &error) {
        std::cerr << "HueAPIResponseException while switching off light: " << error.what() << "\n";
    } catch (hueplusplus::HueException &error) {
        std::cerr << "HueException while switching off light: " << error.what() << "\n";
    } catch (nlohmann::json::parse_error &error) {
        std::cerr << "nlohmann::json::parse_error while switching off light: " << error.what() << "\n";
    } catch (...) {
        std::cerr << "Unknown error while switching off light\n";
    }

    return isOff;
}