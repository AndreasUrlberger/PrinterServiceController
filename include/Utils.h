#pragma once
#include <stdint.h>
#include <wiringPi.h>

#include <chrono>
#include <functional>
#include <string>

struct PrinterState {
    bool state;
    bool tempControl;
    double progress;
    uint64_t remainingTime;
    int32_t boardTemp;
    int32_t nozzleTemp;
    int32_t innerTopTemp;
    int32_t innerBottomTemp;
    int32_t outerTemp;
    std::string profileName;
    int32_t profileTemp;
    float fanSpeed;
};

class Utils {
   public:
    static uint64_t currentMillis();
    static void sleep(int millis);
    static void callLambda(std::function<void()> lambda) {
        lambda();
    }
};