#pragma once

#include <stdint.h>

#include <functional>
#include <thread>

class ButtonController {
    // VARIABLES.
    const uint8_t buttonPin;

    const std::function<void()> onShortClick;
    const std::function<void()> onLongClick;

    // Min time for a short click, used to software debounce the button.
    const uint64_t minShortPressTime;
    // Max time for a short click.
    const uint64_t maxShortPressTime;
    // Min time for a long click.
    const uint64_t minLongPressTime;

    bool isCurrentlyHigh{false};
    uint64_t latestDown{UINT64_C(0)};
    std::thread* pressedThread{nullptr};

    // FUNCTIONS.
    static void interruptHandler(int gpio, int level, uint32_t tick, void* buttonController);

   public:
    // FUNCTIONS.
    void onEdgeChange(bool buttonLevelIsHigh);

    void awaitLongClick(uint64_t down);

    ButtonController(uint8_t buttonPin, std::function<void()> onShortClick, std::function<void()> onLongClick, uint64_t minShortPressTime = UINT64_C(50), uint64_t maxShortPressTime = UINT64_C(1000), uint64_t minLongPressTime = UINT64_C(1500));

    void start();
};
