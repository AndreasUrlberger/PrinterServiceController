#pragma once

#include <stdint.h> //for uint32_t
#include <thread>
#include <functional>

class PowerButtonObserver {
public:
	virtual void onShutdown() = 0;
	virtual void onShortPress() = 0;
};

class PowerButtonController {
private:
	uint32_t SWITCH = 3;
	uint32_t BUZZER = 19;
	int64_t shortPressTime = 1000L; // max duration for a short click
	int64_t minShortPressTime = 50L; // used to avoid double clicks due to a cheap button
	int64_t longPressTime = 2000L; // min duration for a long click

	bool isDown = false;
	int64_t lastDown = 0;
	std::thread* pressedThread = nullptr;
	PowerButtonObserver* observer = nullptr;

	void longPress();

	void shortPress();

public:
	static PowerButtonController* staticController;
	
	void edgeChanging();	

	void threadFun(int64_t down);

	PowerButtonController();

	void start();

	void setObserver(PowerButtonObserver* observer);
};