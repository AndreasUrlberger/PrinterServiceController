#include "Utils.h"

uint64_t Utils::currentMillis()
{
	return std::chrono::system_clock::now().time_since_epoch().count() / 1'000'000UL;
}

void Utils::sleep(int millis)
{
	delay(millis);
}