#pragma once

#include <stdint.h>  //for uint32_t

#include <functional>
#include <thread>

class ButtonController {
    uint32_t SWITCH = 5;
    int64_t shortPressTime = 1000L;   // max duration for a short click
    int64_t minShortPressTime = 50L;  // used to avoid double clicks due to a cheap button
    int64_t longPressTime = 2000L;    // min duration for a long click

    bool isDown = false;
    int64_t lastDown = 0;
    std::thread* pressedThread = nullptr;
    std::function<void(bool longClick)> callback;

    void longPress();

    void shortPress();

   public:
    inline static ButtonController* staticController;

    void edgeChanging();

    void threadFun(int64_t down);

    ButtonController(std::function<void(bool longClick)> callback);

    void start();
};
