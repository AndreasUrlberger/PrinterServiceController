#include "ServiceController.h"

#include <stdint.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>

#include "Buzzer.h"
#include "Logger.h"

// practically a function alias since most compilers will directly call PrintConfigs::getPrintConfigs
constexpr auto printConfigs = PrintConfigs::getPrintConfigs;

void ServiceController::run() {
    state.boardTemp = 0;
    state.nozzleTemp = 0;
    state.progress = 0;
    state.remainingTime = 0;
    state.state = true;
    state.tempControl = fanController.isControlOn();
    state.fanSpeed = 0;

    powerButtonController.start();
    buttonController.start();

    std::thread timerThread = std::thread([this]() {
        auto start = std::chrono::steady_clock::now();
        auto chronoInterval = std::chrono::seconds(1);
        auto targetTime = start + chronoInterval;
        while (true) {
            if ((Utils::currentMillis() - lastActivity) > MAX_INACTIVE_TIME) {
                stopStream();
            }

            std::this_thread::sleep_until(targetTime);
            targetTime += chronoInterval;
        }
    });

    std::thread *displayTempLoopThread = new std::thread([this]() { displayTempLoop(); });

    printerServer.start();
    std::cout << "PrinterServerThread ended\n";
    displayTempLoopThread->join();
    timerThread.join();

    delete displayTempLoopThread;
}

int ServiceController::displayTempLoop() {
    displayController.setInverted(false);

    while (true) {
        state.innerTemp = readTemp(INNER_THERMO_NAME);
        // state.outerTemp = readTemp(OUTER_THERMO_NAME);
        state.outerTemp = 0;
        updateDisplay();

        std::ostringstream ss;
        ss << "inner: " << state.innerTemp << " outer: " << state.outerTemp << " wanted: " << state.profileTemp;
        Logger::log(ss.str());

        printerServer.updateState(state);
        fanController.tempChanged(state.innerTemp, state.profileTemp);
        Utils::sleep(100);
    }
    return 0;
}

void ServiceController::updateDisplay() {
    PrintConfig config = printConfigs()[0];
    state.profileTemp = config.temperature;
    state.profileName = config.name;
    const int64_t now = Utils::currentMillis();
    if (turnOffTime - now > 0 && !shuttingDown) {
        displayController.drawTemperature(state.profileTemp, state.innerTemp, config.name);
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

void ServiceController::onShortPress() {
    turnOffTime = Utils::currentMillis() + SCREEN_ALIVE_TIME;
    updateDisplay();
    displayController.turnOn();
}

void ServiceController::onFanStateChanged(bool isOn) {
    if (isOn) {
        Logger::log(std::string{"fanState: on"});
    } else {
        Logger::log(std::string{"fanState: off"});
    }
    displayController.setIconVisible(isOn);
}

bool ServiceController::onProfileUpdate(PrintConfig &profile) {
    const bool hasChanged = PrintConfigs::addConfig(profile);
    // update server
    state.profileName = profile.name;
    state.profileTemp = profile.temperature;
    printerServer.updateState(state);
    return hasChanged;
}

void ServiceController::onSecondButtonClick(bool longClick) {
    turnOffTime = Utils::currentMillis() + SCREEN_ALIVE_TIME;
    if (longClick) {
        Buzzer::singleBuzz();
        fanController.toggleControl();
    } else {
        // switch config
        auto configs = printConfigs();
        auto nextConfig = configs[configs.size() - 1];
        PrintConfigs::addConfig(nextConfig);
        // manually trigger display
        updateDisplay();
    }
    displayController.turnOn();
}

void ServiceController::onChangeFanControl(bool isOn) {
    if (fanController.isControlOn() xor isOn) {
        fanController.toggleControl();
        state.tempControl = fanController.isControlOn();
        printerServer.updateState(state);
    }
}

void ServiceController::onServerActivity() {
    startStream();

    const uint64_t currentTime = Utils::currentMillis();
    lastActivity = currentTime;
}

void ServiceController::stopStream() {
    if (isStreamRunning) {
        isStreamRunning = false;

        const int result = system("kill $(pidof mjpg_streamer)");
    }
}

void ServiceController::startStream() {
    if (!isStreamRunning) {
        // TODO Add mutex
        isStreamRunning = true;

        const int result = system("nohup mjpg_streamer -o \"output_http.so -w ./www -p 8000\" -i \"input_uvc.so -r 1280x720 -f 1\" > /dev/null 2>&1 &");
    }
}