#include "ServiceController.hpp"

#include <stdint.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>

#include "Logger.hpp"

// practically a function alias since most compilers will directly call PrintConfigs::getPrintConfigs
constexpr auto printConfigs = PrintConfigs::getPrintConfigs;

ServiceController::ServiceController() {
    state.addListener(this);
}

void ServiceController::onPrinterStateChanged() {
    // TODO update display?

    // TODO subscribe display controller to state?.
    displayController.setIconVisible(state.getIsTempControlActive());
}

void ServiceController::run() {
    state.setIsOn(true, false);
    state.setIsTempControlActive(false, false);
    state.setFanSpeed(0, true);

    powerButtonController.start();
    actionButtonController.start();
    lightController.start();
    fanController.start();

    std::thread timerThread = std::thread([this]() {
        Timing::runEveryNSeconds(UINT64_C(1), [this]() {
            if ((Timing::currentTimeMillis() - lastActivity) > MAX_INACTIVE_TIME) {
                stopStream();
            }
        });
    });

    std::thread displayTempLoopThread = std::thread([this]() { displayTempLoop(); });

    printerServer.start();
    displayTempLoopThread.join();
    timerThread.join();
}

int ServiceController::displayTempLoop() {
    displayController.setInverted(false);

    Timing::runEveryNMillis(UINT64_C(1000), [this]() {
        state.setInnerTopTemp(readTemp(INNER_TOP_THERMO_NAME), false);
        state.setInnerBottomTemp(readTemp(INNER_BOTTOM_THERMO_NAME), false);
        state.setOuterTemp(readTemp(OUTER_THERMO_NAME), true);
        updateDisplay();

        std::ostringstream ss;
        ss << "inner: " << state.getInnerTopTemp() << " outer: " << state.getOuterTemp() << " wanted: " << state.getProfileTemp();
        Logger::log(ss.str());
    });

    return 0;
}

void ServiceController::updateDisplay() {
    PrintConfig config = printConfigs()[0];
    state.setProfileTemp(config.temperature, false);
    state.setProfileName(config.name, true);
    const int64_t now = Timing::currentTimeMillis();
    if (turnOffTime - now > 0 && !shuttingDown) {
        displayController.drawTemperature(state.getProfileTemp(), state.getInnerTopTemp(), config.name);
    } else {
        displayController.turnOff();
    }
}

int32_t ServiceController::readTemp(std::string deviceName) {
    // read file to string
    const std::string path = "/sys/bus/w1/devices/" + deviceName + "/w1_slave";
    auto stream = std::ifstream{path.data()};
    std::ostringstream sstr;
    sstr << stream.rdbuf();
    stream.close();
    std::string input = sstr.str();

    // find temp
    const size_t index = input.find("t=") + 2;  // start of temperature
    return index < input.length() ? stoi(input.substr(index)) : 0;
}

void ServiceController::onShutdown() {
    shuttingDown = true;
    PrintConfigs::savePrintConfigs();
    Logger::close();
    turnOffTime = 0;
    displayController.turnOff();
    system("sudo shutdown -h now");
}

bool ServiceController::onProfileUpdate(PrintConfig &profile) {
    const bool hasChanged = PrintConfigs::addConfig(profile);
    // update server
    state.setProfileName(profile.name, false);
    state.setProfileTemp(profile.temperature, true);
    return hasChanged;
}

void ServiceController::onPowerButtonShortClick() {
    keepDisplayAlive();
}

void ServiceController::onPowerButtonLongClick() {
    onShutdown();
}

void ServiceController::onActionButtonShortClick() {
    // Switch config
    auto configs = printConfigs();
    auto nextConfig = configs[configs.size() - 1];
    PrintConfigs::addConfig(nextConfig);

    keepDisplayAlive();
}

void ServiceController::onActionButtonLongClick() {
    buzzer.singleBuzz();
    fanController.toggleControl();

    keepDisplayAlive();
}

void ServiceController::keepDisplayAlive() {
    turnOffTime = Timing::currentTimeMillis() + SCREEN_ALIVE_TIME;
    updateDisplay();
    displayController.turnOn();
}

void ServiceController::onServerActivity() {
    startStream();

    const uint64_t currentTime = Timing::currentTimeMillis();
    lastActivity = currentTime;
}

void ServiceController::startStream() {
    streamMutex.lock();
    if (!isStreamRunning) {
        isStreamRunning = true;
        streamMutex.unlock();

        const int result = system("nohup mjpg_streamer -o \"output_http.so -w ./www -p 8000\" -i \"input_uvc.so -r 1280x720 -f 1 -softfps 1\" > /dev/null 2>&1 &");
        if (result != 0) {
            std::cerr << "ERROR: Starting the stream returned " << result << "\n";
        }

        if (!lightController.switchOn()) {
            std::cerr << "ERROR: Could not switch on light\n";
        }

    } else {
        streamMutex.unlock();
    }
}

void ServiceController::stopStream() {
    streamMutex.lock();
    if (isStreamRunning) {
        isStreamRunning = false;
        streamMutex.unlock();

        const int result = system("kill $(pidof mjpg_streamer)");
        if (result != 0) {
            std::cerr << "ERROR: Stopping the stream returned " << result << "\n";
        }

        if (!lightController.switchOff()) {
            std::cerr << "ERROR: Could not switch off light\n";
        }

    } else {
        streamMutex.unlock();
    }
}