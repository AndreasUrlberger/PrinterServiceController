#pragma once
#include <stdint.h>
#include <string>
#include <chrono>
#include <wiringPi.h>

struct PrinterState {
	bool state;
	double progress;
	uint64_t remainingTime;
	int32_t boardTemp;
	int32_t nozzleTemp;
	int32_t innerTemp;
	int32_t outerTemp;
	std::string profileName;
	int32_t profileTemp;
};

class Utils {
public:
	static uint64_t currentMillis();
	static void sleep(int millis);
};