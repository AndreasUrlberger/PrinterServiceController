#pragma once

#include <mutex>

#include "ButtonController.h"
#include "DisplayController.h"
#include "FanController.h"
#include "HttpProtoServer.h"
#include "PowerButtonController.h"
#include "PrintConfigs.h"
#include "PrinterServer.h"
#include "Utils.h"

static constexpr auto INNER_THERMO_NAME = "28-2ca0a72153ff";
static constexpr auto OUTER_THERMO_NAME = "28-baa0a72915ff";

class ServiceController {
   private:
    static constexpr uint64_t SCREEN_ALIVE_TIME = 0'000;
    static constexpr uint64_t MAX_INACTIVE_TIME = 5'000;
    int64_t turnOffTime = Utils::currentMillis() + SCREEN_ALIVE_TIME;
    bool shuttingDown = false;
    uint64_t lastActivity = Utils::currentMillis();
    bool isStreamRunning = false;
    std::mutex streamMutex;

    int displayTempLoop();
    void updateDisplay();
    int32_t readTemp(std::string deviceName);
    void onShutdown();
    void onShortPress();
    void onFanStateChanged(bool state);
    bool onProfileUpdate(PrintConfig &profile);
    void onSecondButtonClick(bool longClick);
    void onChangeFanControl(bool isOn);
    void onServerActivity();

    void stopStream();
    void startStream();

    PrinterState state;

    PowerButtonController powerButtonController{
        [this]() { onShutdown(); },
        [this]() { onShortPress(); }};
    DisplayController displayController;
    FanController fanController{
        [this](bool state) { onFanStateChanged(state); },
        [this](float fanSpeed) { state.fanSpeed = fanSpeed; }};
    HttpProtoServer printerServer{
        [this]() { onShutdown(); },
        [this](PrintConfig &config) { return onProfileUpdate(config); },
        [this](bool isOn) { onChangeFanControl(isOn); },
        [this]() { onServerActivity(); }};
    ButtonController buttonController{
        [this](bool longClick) { onSecondButtonClick(longClick); }};

   public:
    void run();
};
