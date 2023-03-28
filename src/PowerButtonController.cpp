#include "PowerButtonController.h"

#include <pigpio.h>

#include <iostream>

#include "Buzzer.h"
#include "Utils.h"

void PowerButtonController::longPress() {
    std::thread doubleBuzzer = std::thread(Buzzer::doubleBuzz);
    doubleBuzzer.detach();
    onLongPressHook();
}

void PowerButtonController::shortPress() {
    onShortPressHook();
}

void PowerButtonController::threadFun(int64_t down) {
    // realDownTime is the time when the button was pressed down, the time this thread was started.
    int64_t realDownTime = down;
    std::this_thread::sleep_for(std::chrono::milliseconds(longPressTime));
    const bool anyPressesInBetween = lastDown != realDownTime;
    if (isLevelHigh && !anyPressesInBetween) {
        longPress();
    }
}

void PowerButtonController::edgeChanging(bool buttonLevelIsHigh) {
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

PowerButtonController::PowerButtonController(std::function<void(void)> onLongPress, std::function<void(void)> onShortPress) {
    onLongPressHook = onLongPress;
    onShortPressHook = onShortPress;
}

void PowerButtonController::start() {
    if (gpioInitialise() == PI_INIT_FAILED) {
        std::cerr << "ERROR: gpioInitialise failed in ButtonController::start\n";
        return;
    }

    gpioSetMode(SWITCH, PI_INPUT);
    gpioSetPullUpDown(SWITCH, PI_PUD_DOWN);
    gpioSetISRFuncEx(SWITCH, EITHER_EDGE, 0, interruptHandler, this);

    // Buzz to indicate startup
    std::thread* buzz = new std::thread(Buzzer::singleBuzz);
    buzz->detach();
}

void PowerButtonController::interruptHandler(int gpio, int level, uint32_t tick, void* controller) {
    if (controller != nullptr) {
        PowerButtonController* const buttonController = static_cast<PowerButtonController*>(controller);
        buttonController->edgeChanging(level == 1);
    }
}