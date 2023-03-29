#pragma once

#include <mutex>

#include "ButtonController.hpp"
#include "Buzzer.hpp"
#include "DisplayController.hpp"
#include "FanController.hpp"
#include "HttpProtoServer.hpp"
#include "LightController.hpp"
#include "PrintConfigs.hpp"
#include "Timing.hpp"

class ServiceController {
   private:
    static constexpr auto INNER_TOP_THERMO_NAME{"28-2ca0a72153ff"};
    static constexpr auto INNER_BOTTOM_THERMO_NAME{"28-3c290457da46"};
    static constexpr auto OUTER_THERMO_NAME{"28-baa0a72915ff"};

    static constexpr uint64_t SCREEN_ALIVE_TIME{UINT64_C(30'000)};
    static constexpr uint64_t MAX_INACTIVE_TIME{UINT64_C(3'000)};

    static constexpr auto BRIDGE_IP{"192.168.178.92"};
    static constexpr auto BRIDGE_USERNAME{"yXEXyXC5BYDJbmS-6yNdji6qcatY6BedJmRIb4kO"};
    static constexpr uint8_t PRINTER_LIGHT_ID{UINT8_C(9)};

    int64_t turnOffTime{Timing::currentTimeMillis() + SCREEN_ALIVE_TIME};
    bool shuttingDown{false};
    uint64_t lastActivity{Timing::currentTimeMillis()};
    bool isStreamRunning{false};
    std::mutex streamMutex{};

    int displayTempLoop();
    void updateDisplay();
    int32_t readTemp(std::string deviceName);
    void onShutdown();
    void onFanStateChanged(bool state);
    bool onProfileUpdate(PrintConfig &profile);
    void onPowerButtonShortClick();
    void onPowerButtonLongClick();
    void onActionButtonShortClick();
    void onActionButtonLongClick();
    void keepDisplayAlive();
    void onChangeFanControl(bool isOn);
    void onServerActivity();

    void stopStream();
    void startStream();

    PrinterState state;

    Buzzer buzzer{UINT8_C(26)};
    ButtonController powerButtonController{
        UINT8_C(3),
        [this]() { onPowerButtonShortClick(); },
        [this]() { onPowerButtonLongClick(); }};
    DisplayController displayController;
    HttpProtoServer printerServer{
        [this]() { onShutdown(); },
        [this](PrintConfig &config) { return onProfileUpdate(config); },
        [this](bool isOn) { onChangeFanControl(isOn); },
        [this]() { onServerActivity(); }};
    ButtonController actionButtonController{
        // Button pin
        UINT8_C(5),
        [this]() { onActionButtonShortClick(); },
        [this]() { onActionButtonLongClick(); }};
    LightController lightController{BRIDGE_IP, BRIDGE_USERNAME, PRINTER_LIGHT_ID};
    FanController fanController{
        [this](bool state) { onFanStateChanged(state); },
        [this](float fanSpeed) { state.setFanSpeed(fanSpeed); }};

   public:
    void run();
};
