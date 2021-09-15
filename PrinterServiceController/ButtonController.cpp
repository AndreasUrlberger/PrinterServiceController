#include "ButtonController.h"
#include <wiringPi.h>
#include "Utils.h"


void interruptWrapper2() {
	if (ButtonController::staticController != nullptr) {
		ButtonController::staticController->edgeChanging();
	}
}

void threadTimerWrapper2(uint64_t down) {
	if (ButtonController::staticController != nullptr) {
		ButtonController::staticController->threadFun(down);
	}
}

void ButtonController::longPress() {
	callback(true);
}

void ButtonController::shortPress() {
	callback(false);
}

void ButtonController::threadFun(int64_t down) {
	int64_t realDownTime = down;
	std::this_thread::sleep_for(std::chrono::milliseconds(longPressTime));
	if (isDown && realDownTime == lastDown) {
		longPress();
	}
}

void ButtonController::edgeChanging() {
	int state = digitalRead(SWITCH);
	isDown = state == 0;
	if (isDown) {
		isDown = true;
		lastDown = std::chrono::system_clock::now().time_since_epoch().count();
		if (pressedThread != nullptr) {
			delete pressedThread;
		}
		pressedThread = new std::thread(threadTimerWrapper2, lastDown);
		pressedThread->detach();
	}
	else {
		int64_t now = std::chrono::system_clock::now().time_since_epoch().count();
		int64_t downTime = (now - lastDown) / 1'000'000L;
		if (downTime < shortPressTime && downTime >= minShortPressTime) {
			shortPress();
		}
	}
}

ButtonController::ButtonController(std::function<void(bool longClick)> callback)
{
	this->callback = callback;
}

void ButtonController::start() {
	ButtonController::staticController = this;
	wiringPiSetupSys();

	pinMode(SWITCH, INPUT);
	pullUpDnControl(SWITCH, PUD_DOWN);
	wiringPiISR(SWITCH, INT_EDGE_BOTH, interruptWrapper2);
}
