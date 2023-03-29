#include "ButtonController.hpp"

#include <pigpio.h>

#include <iostream>

#include "Timing.hpp"

void ButtonController::awaitLongClick(uint64_t timeAtClick) {
    // originalDownTime is the time when the button was pressed down, the time this thread was started.
    const uint64_t originalDownTime = timeAtClick;
    Timing::sleepMillis(minLongPressTime);
    const bool anyPressesInBetween = latestDown != originalDownTime;
    if (isCurrentlyHigh && !anyPressesInBetween) {
        onLongClick();
    }
}

void ButtonController::onEdgeChange(bool buttonLevelIsHigh) {
    isCurrentlyHigh = buttonLevelIsHigh;
    const uint64_t currenTimeNs = Timing::currentTimeNanos();

    if (isCurrentlyHigh) {
        latestDown = currenTimeNs;
        if (pressedThread != nullptr) {
            delete pressedThread;
        }
        pressedThread = new std::thread([this]() { awaitLongClick(latestDown); });
        pressedThread->detach();
    } else {
        const uint64_t downTimeMs = (currenTimeNs - latestDown) / UINT64_C(1'000'000);
        if (downTimeMs < maxShortPressTime && downTimeMs >= minShortPressTime) {
            onShortClick();
        }
    }
}

ButtonController::ButtonController(uint8_t buttonPin, std::function<void()> onShortClick, std::function<void()> onLongClick, uint64_t minShortPressTime, uint64_t maxShortPressTime, uint64_t minLongPressTime) : buttonPin(buttonPin), onShortClick(onShortClick), onLongClick(onLongClick), minShortPressTime(minShortPressTime), maxShortPressTime(maxShortPressTime), minLongPressTime(minLongPressTime) {
}

void ButtonController::start() {
    if (gpioInitialise() == PI_INIT_FAILED) {
        std::cerr << "ERROR: gpioInitialise failed in ButtonController::start\n";
        return;
    }

    gpioSetMode(buttonPin, PI_INPUT);
    gpioSetPullUpDown(buttonPin, PI_PUD_DOWN);
    gpioSetISRFuncEx(buttonPin, EITHER_EDGE, 0, interruptHandler, this);
}

void ButtonController::interruptHandler(int gpio, int level, uint32_t tick, void* buttonController) {
    if (buttonController != nullptr) {
        ((ButtonController*)buttonController)->onEdgeChange(level == 1);
    }
}
