#include "Timing.hpp"

#include <chrono>
#include <thread>

void Timing::runEveryNSeconds(uint64_t seconds, std::function<void*(void*)> executable) {
    runEveryNMillis(seconds * UINT64_C(1000), executable);
}

void Timing::runEveryNMillis(uint64_t millis, std::function<void*(void*)> executable) {
}

void Timing::sleepSeconds(uint64_t seconds) {
    sleepMillis(seconds * UINT64_C(1000));
}

void Timing::sleepMillis(uint64_t millis) {
    std::this_thread::sleep_for(std::chrono::milliseconds(millis));
}

uint64_t Timing::currentTimeSeconds() {
    return currentTimeNanos() / UINT64_C(1'000'000'000);
}

uint64_t Timing::currentTimeMillis() {
    return currentTimeNanos() / UINT64_C(1'000'000);
}

uint64_t Timing::currentTimeNanos() {
    return std::chrono::system_clock::now().time_since_epoch().count();
}