#include "Utils.h"

#include <chrono>
#include <thread>

uint64_t Utils::currentMillis() {
    return std::chrono::system_clock::now().time_since_epoch().count() / 1'000'000UL;
}

void Utils::sleep(int millis) {
    std::this_thread::sleep_for(std::chrono::milliseconds(millis));
}