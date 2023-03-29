#pragma once
#include <stdint.h>

#include <functional>
#include <string>

struct PrinterState {
    bool isOn;
    bool isTempControlActive;
    double printProgress;
    uint64_t remainingTime;
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
