#include "PowerButtonController.h"
#include <wiringPi.h>
#include "Utils.h"
#include "Buzzer.h"

void interruptWrapper() {
	if (PowerButtonController::staticController != nullptr) {
		PowerButtonController::staticController->edgeChanging();
	}
}

void PowerButtonController::longPress() {
	std::thread doubleBuzzer = std::thread(Buzzer::doubleBuzz);
	doubleBuzzer.detach();
	onLongPressHook();
}

void PowerButtonController::shortPress() {
	onShortPressHook();
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
		pressedThread = new std::thread([this]() {threadFun(lastDown); });
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

PowerButtonController::PowerButtonController(std::function<void(void)> onLongPress, std::function<void(void)> onShortPress)
{
	onLongPressHook = onLongPress;
	onShortPressHook = onShortPress;
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
