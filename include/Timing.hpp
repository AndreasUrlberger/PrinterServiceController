#pragma once

#include <stdint.h>

#include <functional>

class Timing {
   public:
    Timing() = delete;
    ~Timing() = delete;

    static void runEveryNSeconds(uint64_t seconds, std::function<void*(void*)> executable);

    static void runEveryNMillis(uint64_t millis, std::function<void*(void*)> executable);

    static void sleepSeconds(uint64_t seconds);

    static void sleepMillis(uint64_t millis);

    static uint64_t currentTimeSeconds();

    static uint64_t currentTimeMillis();

    static uint64_t currentTimeNanos();
};
