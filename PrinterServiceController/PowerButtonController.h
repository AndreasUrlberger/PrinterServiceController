#pragma once

#include <stdint.h> //for uint32_t
#include <thread>
#include <functional>

class PowerButtonController {
private:
	uint32_t SWITCH = 3;
	int64_t shortPressTime = 1000L; // max duration for a short click
	int64_t minShortPressTime = 30'000L; // normally at 50 // used to avoid double clicks due to a cheap button
	int64_t longPressTime = 2000L; // min duration for a long click

	bool isDown = false;
	int64_t lastDown = 0;
	std::thread* pressedThread = nullptr;
	std::function<void(void)> onLongPressHook;
	std::function<void(void)> onShortPressHook;

	void longPress();

	void shortPress();

public:
	inline static PowerButtonController* staticController;
	
	void edgeChanging();	

	void threadFun(int64_t down);

	PowerButtonController(std::function<void(void)> onLongPress, std::function<void(void)> onShortPress);

	void start();
};