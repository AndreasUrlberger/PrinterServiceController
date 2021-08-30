#pragma once
#include <stdint.h>
#include <chrono>
#include <wiringPi.h>

class Utils {
public:
	static uint64_t currentMillis();
	static void sleep(int millis);
};