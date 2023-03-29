#pragma once

#include <hueplusplus/Bridge.h>

#include <memory>

class LightController {
   private:
    static constexpr auto BRIDGE_IP = "192.168.178.92";
    static constexpr auto BRIDGE_USERNAME = "yXEXyXC5BYDJbmS-6yNdji6qcatY6BedJmRIb4kO";
    static constexpr int PRINTER_LIGHT_ID = 9;

    std::unique_ptr<hueplusplus::Light> printerLight;

   public:
    LightController();
    void start();
    bool tryGetLight();
    bool switchOn();
    bool switchOff();
};