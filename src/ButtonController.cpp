#include "ButtonController.hpp"

#include <pigpio.h>

#include <iostream>

#include "Utils.hpp"

void ButtonController::longPress() {
    callback(true);
}

void ButtonController::shortPress() {
    callback(false);
}

void ButtonController::threadFun(int64_t down) {
    // realDownTime is the time when the button was pressed down, the time this thread was started.
    const int64_t realDownTime = down;
    std::this_thread::sleep_for(std::chrono::milliseconds(longPressTime));
    const bool anyPressesInBetween = lastDown != realDownTime;
    if (isLevelHigh && !anyPressesInBetween) {
        longPress();
    }
}

void ButtonController::edgeChanging(bool buttonLevelIsHigh) {
    isLevelHigh = buttonLevelIsHigh;

    if (isLevelHigh) {
        lastDown = std::chrono::system_clock::now().time_since_epoch().count();
        if (pressedThread != nullptr) {
            delete pressedThread;
        }
        pressedThread = new std::thread([this]() { threadFun(lastDown); });
        pressedThread->detach();
    } else {
        const int64_t now = std::chrono::system_clock::now().time_since_epoch().count();
        const int64_t downTimeMs = (now - lastDown) / 1'000'000L;
        if (downTimeMs < maxShortPressTime && downTimeMs >= minShortPressTime) {
            shortPress();
        }
    }
}

ButtonController::ButtonController(std::function<void(bool longClick)> callback) {
    this->callback = callback;
}

void ButtonController::start() {
    if (gpioInitialise() == PI_INIT_FAILED) {
        std::cerr << "ERROR: gpioInitialise failed in ButtonController::start\n";
        return;
    }

    gpioSetMode(SWITCH, PI_INPUT);
    gpioSetPullUpDown(SWITCH, PI_PUD_DOWN);
    gpioSetISRFuncEx(SWITCH, EITHER_EDGE, 0, interruptHandler, this);
}

void ButtonController::interruptHandler(int gpio, int level, uint32_t tick, void* buttonController) {
    if (buttonController != nullptr) {
        ((ButtonController*)buttonController)->edgeChanging(level == 1);
    }
}
