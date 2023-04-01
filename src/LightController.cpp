#include "LightController.hpp"

#include <hueplusplus/LinHttpHandler.h>

// Constructor setting the parameters in constructor initializer list
LightController::LightController(const std::string bridgeIp, const std::string bridgeUsername, const int lightId)
    : bridgeIp(bridgeIp), bridgeUsername(bridgeUsername), lightId(lightId) {}

void LightController::start() {
    tryGetLight();
}

bool LightController::tryGetLight() {
    const auto handler = std::make_shared<hueplusplus::LinHttpHandler>();
    hueplusplus::Bridge bridge(bridgeIp, 80, bridgeUsername, handler);

    printerLight = nullptr;
    try {
        printerLight = std::make_unique<hueplusplus::Light>(bridge.lights().get(lightId));
    } catch (const std::system_error &error) {
        std::cerr << "Error while getting light: " << error.what() << "\n";
    } catch (const hueplusplus::HueAPIResponseException &error) {
        std::cerr << "HueAPIResponseException while getting light: " << error.what() << "\n";
    } catch (const hueplusplus::HueException &error) {
        std::cerr << "HueException while getting light: " << error.what() << "\n";
    } catch (const nlohmann::json::parse_error &error) {
        std::cerr << "nlohmann::json::parse_error while getting light: " << error.what() << "\n";
    } catch (const std::exception &error) {
        std::cerr << "std::exception while getting light: " << error.what() << "\n";
    }

    return printerLight != nullptr;
}

// Switch light on and handle all exceptions. Return true if successfull.
bool LightController::switchOn() {
    bool isOn = false;

    if (printerLight == nullptr) {
        if (!tryGetLight()) {
            std::cerr << "ERROR: Could not get light, therefore could not switch it on\n";
            return false;
        }
    }

    try {
        printerLight->on();
        isOn = true;
    } catch (const std::system_error &error) {
        std::cerr << "Error while switching on light: " << error.what() << "\n";
    } catch (const hueplusplus::HueAPIResponseException &error) {
        std::cerr << "HueAPIResponseException while switching on light: " << error.what() << "\n";
    } catch (const hueplusplus::HueException &error) {
        std::cerr << "HueException while switching on light: " << error.what() << "\n";
    } catch (const nlohmann::json::parse_error &error) {
        std::cerr << "nlohmann::json::parse_error while switching on light: " << error.what() << "\n";
    } catch (...) {
        std::cerr << "Unknown error while switching on light\n";
    }

    return isOn;
}

// Switch light off and handle all exceptions. Return true if successfull.
bool LightController::switchOff() {
    bool isOff = false;

    if (printerLight == nullptr) {
        if (!tryGetLight()) {
            std::cerr << "ERROR: Could not get light, therefore could not switch it off\n";
            return false;
        }
    }

    try {
        printerLight->off();
        isOff = true;
    } catch (const std::system_error &error) {
        std::cerr << "Error while switching off light: " << error.what() << "\n";
    } catch (const hueplusplus::HueAPIResponseException &error) {
        std::cerr << "HueAPIResponseException while switching off light: " << error.what() << "\n";
    } catch (const hueplusplus::HueException &error) {
        std::cerr << "HueException while switching off light: " << error.what() << "\n";
    } catch (const nlohmann::json::parse_error &error) {
        std::cerr << "nlohmann::json::parse_error while switching off light: " << error.what() << "\n";
    } catch (...) {
        std::cerr << "Unknown error while switching off light\n";
    }

    return isOff;
}