#include "PowerButtonController.h"
#include <wiringPi.h>
#include "Utils.h"
#include "Buzzer.h"

void interruptWrapper() {
	if (PowerButtonController::staticController != nullptr) {
		PowerButtonController::staticController->edgeChanging();
	}
}

void threadTimerWrapper(uint64_t down) {
	if (PowerButtonController::staticController != nullptr) {
		PowerButtonController::staticController->threadFun(down);
	}
}

void PowerButtonController::longPress() {
	std::thread doubleBuzzer = std::thread(Buzzer::doubleBuzz);
	doubleBuzzer.detach();
	if (observer != nullptr)
		observer->onShutdown();
	system("sudo shutdown -h now");
}

void PowerButtonController::shortPress() {
	if (observer != nullptr) {
		observer->onShortPress();
	}
}

void PowerButtonController::threadFun(int64_t down) {
	int64_t realDownTime = down;
	std::this_thread::sleep_for(std::chrono::milliseconds(longPressTime));
	if (isDown && realDownTime == lastDown) {
		longPress();
	}
}

void PowerButtonController::edgeChanging() {
	int state = digitalRead(SWITCH);
	isDown = state == 0;
	if (isDown) {
		isDown = true;
		lastDown = std::chrono::system_clock::now().time_since_epoch().count();
		if (pressedThread != nullptr) {
			delete pressedThread;
		}
		pressedThread = new std::thread(threadTimerWrapper, lastDown);
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

PowerButtonController::PowerButtonController()
{
	
}

void PowerButtonController::start() {
	PowerButtonController::staticController = this;
	wiringPiSetupSys();

	pinMode(SWITCH, INPUT);
	pullUpDnControl(SWITCH, PUD_DOWN);
	wiringPiISR(SWITCH, INT_EDGE_BOTH, interruptWrapper);

	// Buzz to indicate startup
	std::thread* buzz = new std::thread(Buzzer::singleBuzz);
	buzz->detach();
}

void PowerButtonController::setObserver(PowerButtonObserver* observer)
{
	this->observer = observer;
}
