#include "PowerButtonController.h"
#include <wiringPi.h>
#include "Utils.h"

PowerButtonController* PowerButtonController::staticController;

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

void singleBuzz(uint32_t pin) {
	digitalWrite(pin, HIGH);
	delay(333);
	digitalWrite(pin, LOW);
}

void doubleBuzz(uint32_t pin) {
	digitalWrite(pin, HIGH);
	delay(80);
	digitalWrite(pin, LOW);
	delay(30);
	digitalWrite(pin, HIGH);
	delay(80);
	digitalWrite(pin, LOW);
}

void PowerButtonController::longPress() {
	std::thread doubleBuzzer = std::thread(doubleBuzz, BUZZER);
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
	pinMode(BUZZER, OUTPUT);
	pullUpDnControl(SWITCH, PUD_DOWN);
	wiringPiISR(SWITCH, INT_EDGE_BOTH, interruptWrapper);

	// Buzz to indicate startup
	std::thread* buzz = new std::thread(singleBuzz, BUZZER);
	buzz->detach();
}

void PowerButtonController::setObserver(PowerButtonObserver* observer)
{
	this->observer = observer;
}
