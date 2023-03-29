#pragma once

#include <hueplusplus/Bridge.h>

#include <memory>

class LightController {
   private:
    // VARIABLES
    const std::string bridgeIp;
    const std::string bridgeUsername;
    const uint32_t lightId;

    std::unique_ptr<hueplusplus::Light> printerLight;

    // FUNCTIONS
    bool tryGetLight();

   public:
    LightController(const std::string bridgeIp, const std::string bridgeUsername, const int lightId);
    void start();
    bool switchOn();
    bool switchOff();
};