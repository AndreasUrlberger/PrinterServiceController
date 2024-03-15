#pragma once

#include <mutex>
#include <string>

#include "ButtonController.hpp"
#include "Buzzer.hpp"
#include "DisplayController.hpp"
#include "FanController.hpp"
#include "HttpProtoServer.hpp"
#include "LightController.hpp"
#include "PrintConfigs.hpp"
#include "Timing.hpp"

class ServiceController : private PrinterState::PrinterStateListener
{

public:
    struct ThermometerConfig
    {
        const std::string innerTopThermoName;
        const std::string innerBottomThermoName;
        const std::string outerThermoName;
    };

    struct GeneralConfig
    {
        const uint64_t screenAliveTime;
        const uint64_t maxInactiveTime;
    };

    struct LightConfig
    {
        const std::string bridgeIp;
        const std::string bridgeUsername;
        const uint8_t printerLightId;
    };

    struct DisplayConfig
    {
        const uint8_t height;
        const uint8_t width;
        const std::string fileName;
        const uint8_t fontSize;
        const std::string fontName;
    };

    struct HttpServerConfig
    {
        const uint16_t port;
    };

    struct CameraConfig
    {
        const std::string startCommand;
        const std::string stopCommand;
    };

private:
    const ThermometerConfig thermoConfig;
    const GeneralConfig generalConfig;
    const CameraConfig cameraConfig;

    uint64_t turnOffTime;
    bool shuttingDown{false};
    uint64_t lastActivity{Timing::currentTimeMillis()};
    bool isStreamRunning{false};
    std::mutex streamMutex{};

    int displayTempLoop();
    void updateDisplay();
    int32_t readTemp(std::string deviceName);
    void onShutdown();
    bool onProfileUpdate(PrintConfig &profile);
    void onPowerButtonShortClick();
    void onPowerButtonLongClick();
    void onActionButtonShortClick();
    void onActionButtonLongClick();
    void keepDisplayAlive();
    void onServerActivity();

    void stopStream();
    void startStream();

    PrinterState state{};

    Buzzer buzzer{UINT8_C(26)};
    ButtonController powerButtonController{
        UINT8_C(3),
        [this]()
        { onPowerButtonShortClick(); },
        [this]()
        { onPowerButtonLongClick(); }};
    DisplayController displayController;
    HttpProtoServer printerServer;
    ButtonController actionButtonController{
        // Button pin
        UINT8_C(5),
        [this]()
        { onActionButtonShortClick(); },
        [this]()
        { onActionButtonLongClick(); }};
    LightController lightController;
    FanController fanController;

public:
    ServiceController(const ThermometerConfig &thermoConfig, const GeneralConfig &generalConfig, const CameraConfig &cameraConfig, const LightConfig &lightConfig, const DisplayConfig &displayConfig, const HttpServerConfig &httpServerConfig, const FanController::FanControllerConfig &fanConfig);
    void onPrinterStateChanged() override;

    void run();
};
