#pragma once
#include <stdint.h>
#include <string>
#include <chrono>
#include <wiringPi.h>

struct PrinterState {
	bool state;
	double progress;
	uint64_t remainingTime;
	double boardTemp;
	double nozzleTemp;
	double innerTemp;
	double outerTemp;
	std::string profileName;
	int32_t profileTemp;
};

class Utils {
public:
	static uint64_t currentMillis();
	static void sleep(int millis);
};