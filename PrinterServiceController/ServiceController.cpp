#include "ServiceController.h"
#include <fstream>
#include <sstream>
#include <stdint.h>
#include <iostream>
#include <algorithm>

#define CONFIG_FILE_PATH "/usr/share/printer-service-controller/"
#define CONFIG_FILE_NAME "print-configs.txt"

void ServiceController::run()
{
	PrintConfigs::loadPrintConfigs(CONFIG_FILE_PATH, CONFIG_FILE_NAME, printConfigs);
	fanController.setObserver(this);
	powerButtonController.setObserver(this);
	powerButtonController.start();
	printerServer.setObserver(this);
	printerServer.start();
	displayTempLoop();
}

int ServiceController::displayTempLoop() {
	displayController.setInverted(false);

	while (true) {
		PrintConfig config = printConfigs[0];
		int32_t wantedTemp = config.temperatur;
		int64_t now = Utils::currentMillis();   
		int32_t have = readTemp();
		state.innerTemp = have;
		state.profileTemp = config.temperatur;
		state.profileName = config.name;
		printerServer.setContent(state);
		// TODO: read inner and outer temp
		fanController.tempChanged(have, wantedTemp);
		if (turnOffTime - now > 0 && !shuttingDown) {
			displayController.drawTemperature(wantedTemp, have, config.name);
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
	PrintConfig config = printConfigs[0];
	turnOffTime = Utils::currentMillis() + SCREEN_ALIVE_TIME;
	int have = readTemp();
	displayController.drawTemperature(config.temperatur, have, config.name);
	displayController.turnOn();
}

void ServiceController::onFanStateChanged(bool isOn)
{
	displayController.setIconVisible(isOn);
}

void ServiceController::onProfileUpdate(PrintConfig& profile)
{
	auto profileComp = [&profile](PrintConfig& element) {return element.name.compare(profile.name); };
	auto searchResult = std::find_if(printConfigs.begin(), printConfigs.end(), profileComp);
	if (searchResult == printConfigs.end()) {
		printConfigs.emplace(printConfigs.begin(), profile);
	}
	else {
		int index = searchResult - printConfigs.begin();
		printConfigs.at(index) = profile;
	}
	// update server
	state.profileName = profile.name;
	state.profileTemp = profile.temperatur;
	printerServer.setContent(state);
}
