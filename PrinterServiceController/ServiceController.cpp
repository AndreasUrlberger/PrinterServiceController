#include "ServiceController.h"
#include <fstream>
#include <sstream>
#include <wiringPi.h> // only for the delay function
#include <stdint.h>

#include <iostream>

void ServiceController::run()
{
	powerButtonController.setObserver(this);
	powerButtonController.start();
	printerServer.start();
	displayTempLoop();
}

int ServiceController::displayTempLoop() {
	displayController.setIconVisible(true);
	displayController.setInverted(false);

	while (true) {
		int64_t now = Utils::currentMillis();
		int32_t have = readTemp();
		fanController.tempChanged(have);
		if (turnOffTime - now > 0 && !shuttingDown) {
			displayController.drawTemperature(25, have);
		}
		else {
			displayController.turnOff();
		}
		delay(1'000);
	}

	return 0;
}

int32_t ServiceController::readTemp() {
	// read file to string
	std::string device = "28-2ca0a72153ff";
	std::string path = "/sys/bus/w1/devices/" + device + "/w1_slave";
	auto stream = std::ifstream{ path.data() };
	std::ostringstream sstr;
	sstr << stream.rdbuf();
	stream.close();
	std::string input = sstr.str();

	// find temp
	const int index = input.find("t=") + 2; // start of temperature
	int32_t number = stoi(input.substr(index));
	return number;
}

void ServiceController::onShutdown()
{
	shuttingDown = true;
	turnOffTime = 0;
	displayController.turnOff();
}

void ServiceController::onShortPress() {
	turnOffTime = Utils::currentMillis() + SCREEN_ALIVE_TIME;
	int have = readTemp();
	displayController.drawTemperature(25, have);
	displayController.turnOn();
}
