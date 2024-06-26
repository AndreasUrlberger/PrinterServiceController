#include "ServiceController.hpp"

#include <stdint.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>

#include "Logger.hpp"

ServiceController::ServiceController(const ThermometerConfig &thermoConfig, const GeneralConfig &generalConfig, const CameraConfig &cameraConfig, const LightConfig &lightConfig, const DisplayConfig &displayConfig, const HttpServerConfig &httpServerConfig, const FanController::FanControllerConfig &fanConfig)
    : thermoConfig{thermoConfig},
      generalConfig{generalConfig},
      cameraConfig{cameraConfig},
      displayController{displayConfig.fileName, displayConfig.height, displayConfig.width, displayConfig.fontSize, displayConfig.fontName},
      printerServer{state, httpServerConfig.port, [this]()
                    { onShutdown(); },
                    [this](PrintConfig &config)
                    { return onProfileUpdate(config); },
                    [this]()
                    { onServerActivity(); }},
      lightController{lightConfig.bridgeIp, lightConfig.bridgeUsername, lightConfig.printerLightId},
      fanController{state, fanConfig}
{
    turnOffTime = Timing::currentTimeMillis() + generalConfig.screenAliveTime;
    state.addListener(this);
}

void ServiceController::onPrinterStateChanged()
{
    displayController.setIconVisible(state.getIsTempControlActive());
}

void ServiceController::run()
{
    state.setIsOn(true, false);
    state.setIsTempControlActive(false, false);
    state.setFanSpeed(0, false);
    auto configs = PrintConfigs::getPrintConfigs();
    if (configs.size() > 0)
    {
        state.setProfileTemp(configs[0].temperature, false);
        state.setProfileName(configs[0].name, true);
    }

    powerButtonController.start();
    actionButtonController.start();
    lightController.start();
    fanController.start();

    std::thread timerThread = std::thread([this]()
                                          { Timing::runEveryNSeconds(UINT64_C(1), [this]()
                                                                     {
            if ((Timing::currentTimeMillis() - lastActivity) > generalConfig.maxInactiveTime) {
                stopStream();
            } }); });

    std::thread displayTempLoopThread = std::thread([this]()
                                                    { displayTempLoop(); });

    printerServer.start();
    displayTempLoopThread.join();
    timerThread.join();
}

int ServiceController::displayTempLoop()
{
    displayController.setInverted(false);

    Timing::runEveryNMillis(UINT64_C(1000), [this]()
                            {
        state.setInnerTopTemp(readTemp(thermoConfig.innerTopThermoName), false);
        state.setInnerBottomTemp(readTemp(thermoConfig.innerBottomThermoName), false);
        state.setOuterTemp(readTemp(thermoConfig.outerThermoName), true);
        updateDisplay();

        std::ostringstream ss;
        ss << "inner: " << state.getInnerTopTemp() << " outer: " << state.getOuterTemp() << " wanted: " << state.getProfileTemp();
        Logger::log(ss.str()); });

    return 0;
}

void ServiceController::updateDisplay()
{
    PrintConfig config = PrintConfigs::getPrintConfigs()[0];
    const uint64_t now = Timing::currentTimeMillis();
    if (turnOffTime > now && !shuttingDown)
    {
        displayController.drawTemperature(state.getProfileTemp(), state.getInnerTopTemp(), config.name);
    }
    else
    {
        displayController.turnOff();
    }
}

int32_t ServiceController::readTemp(std::string deviceName)
{
    // read file to string
    const std::string path = "/sys/bus/w1/devices/" + deviceName + "/w1_slave";
    auto stream = std::ifstream{path.data()};
    std::ostringstream sstr;
    sstr << stream.rdbuf();
    stream.close();
    std::string input = sstr.str();

    // find temp
    const size_t index = input.find("t=") + 2; // start of temperature
    return index < input.length() ? stoi(input.substr(index)) : 0;
}

void ServiceController::onShutdown()
{
    shuttingDown = true;
    PrintConfigs::savePrintConfigs();
    Logger::close();
    turnOffTime = 0;
    displayController.turnOff();
    system("sudo shutdown -h now");
}

bool ServiceController::onProfileUpdate(PrintConfig &profile)
{
    const bool hasChanged = PrintConfigs::addConfig(profile);
    // update server
    state.setProfileName(profile.name, false);
    state.setProfileTemp(profile.temperature, true);
    return hasChanged;
}

void ServiceController::onPowerButtonShortClick()
{
    keepDisplayAlive();
}

void ServiceController::onPowerButtonLongClick()
{
    onShutdown();
}

void ServiceController::onActionButtonShortClick()
{
    // Switch config
    auto configs = PrintConfigs::getPrintConfigs();
    auto nextConfig = configs[configs.size() - 1];
    PrintConfigs::addConfig(nextConfig);
    state.setProfileName(nextConfig.name, false);
    state.setProfileTemp(nextConfig.temperature, true);

    keepDisplayAlive();
}

void ServiceController::onActionButtonLongClick()
{
    buzzer.singleBuzz();
    fanController.toggleControl();

    keepDisplayAlive();
}

void ServiceController::keepDisplayAlive()
{
    turnOffTime = Timing::currentTimeMillis() + generalConfig.screenAliveTime;
    updateDisplay();
    displayController.turnOn();
}

void ServiceController::onServerActivity()
{
    startStream();

    const uint64_t currentTime = Timing::currentTimeMillis();
    lastActivity = currentTime;
}

void ServiceController::startStream()
{
    streamMutex.lock();
    if (!isStreamRunning)
    {
        isStreamRunning = true;
        streamMutex.unlock();

        const int result = system(cameraConfig.startCommand.c_str());
        if (result != 0)
        {
            std::cerr << "ERROR: Starting the stream returned " << result << "\n";
        }

        if (!lightController.switchOn())
        {
            std::cerr << "ERROR: Could not switch on light\n";
        }
    }
    else
    {
        streamMutex.unlock();
    }
}

void ServiceController::stopStream()
{
    streamMutex.lock();
    if (isStreamRunning)
    {
        isStreamRunning = false;
        streamMutex.unlock();

        const int result = system(cameraConfig.stopCommand.c_str());
        if (result != 0)
        {
            std::cerr << "ERROR: Stopping the stream returned " << result << "\n";
        }

        if (!lightController.switchOff())
        {
            std::cerr << "ERROR: Could not switch off light\n";
        }
    }
    else
    {
        streamMutex.unlock();
    }
}