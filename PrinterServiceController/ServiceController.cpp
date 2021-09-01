#include "ServiceController.h"
#include <fstream>
#include <sstream>
#include <stdint.h>
#include <iostream>

#define CONFIG_FILE_PATH "/usr/share/printer-service-controller/"
#define CONFIG_FILE_NAME "print-configs.txt"

void ServiceController::run()
{
	PrintConfigs::loadPrintConfigs(CONFIG_FILE_PATH, CONFIG_FILE_NAME, printConfigs);
	fanController.setObserver(this);
	powerButtonController.setObserver(this);
	powerButtonController.start();
	printerServer.start();
	displayTempLoop();
}

int ServiceController::displayTempLoop() {
	displayController.setInverted(false);

	while (true) {
		int64_t now = Utils::currentMillis();
		int32_t have = readTemp();
		fanController.tempChanged(have);
		if (turnOffTime - now > 0 && !shuttingDown) {
			displayController.drawTemperature(25, have);
			printerServer.setContent(false, 0.0, 0, 0.0, 0.0, static_cast<double>(have) / 1000, 0.0, "PETG", 0.0);
		}
		else {
			displayController.turnOff();
		}
		Utils::sleep(1'000);
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
	PrintConfigs::savePrintConfigs(CONFIG_FILE_PATH, CONFIG_FILE_NAME, printConfigs);
	turnOffTime = 0;
	displayController.turnOff();
}

void ServiceController::onShortPress() {
	turnOffTime = Utils::currentMillis() + SCREEN_ALIVE_TIME;
	int have = readTemp();
	displayController.drawTemperature(25, have);
	displayController.turnOn();
}

void ServiceController::onFanStateChanged(bool isOn)
{
	displayController.setIconVisible(isOn);
}
