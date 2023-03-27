#include "ButtonController.h"

#include <pigpio.h>

#include <iostream>

#include "Utils.h"

void ButtonController::longPress() {
    callback(true);
}

void ButtonController::shortPress() {
    callback(false);
}

void ButtonController::threadFun(int64_t down) {
    int64_t realDownTime = down;
    std::this_thread::sleep_for(std::chrono::milliseconds(longPressTime));
    if (isDown && realDownTime == lastDown) {
        longPress();
    }
}

void ButtonController::edgeChanging() {
    int state = gpioRead(SWITCH);
    if (state == PI_BAD_GPIO) {
        std::cerr << "ERROR: gpioRead failed in ButtonController\n";
        return;
    }
    isDown = state == 0;
    if (isDown) {
        isDown = true;
        lastDown = std::chrono::system_clock::now().time_since_epoch().count();
        if (pressedThread != nullptr) {
            delete pressedThread;
        }
        pressedThread = new std::thread([this]() { threadFun(lastDown); });
        pressedThread->detach();
    } else {
        int64_t now = std::chrono::system_clock::now().time_since_epoch().count();
        int64_t downTime = (now - lastDown) / 1'000'000L;
        if (downTime < shortPressTime && downTime >= minShortPressTime) {
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
        ((ButtonController*)buttonController)->edgeChanging();
    }
}
