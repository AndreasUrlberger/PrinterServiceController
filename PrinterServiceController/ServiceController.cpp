#include "ServiceController.h"
#include <fstream>
#include <sstream>
#include <stdint.h>
#include <iostream>
#include "Logger.h"
#include <sstream>
#include "Buzzer.h"

// practically a function alias since most compilers will directly call PrintConfigs::getPrintConfigs
constexpr auto printConfigs = PrintConfigs::getPrintConfigs;

void ServiceController::run()
{
	state.boardTemp = 0;
	state.nozzleTemp = 0;
	state.progress = 0;
	state.remainingTime = 0;
	state.state = false;

	fanController.setObserver(this);
	powerButtonController.setObserver(this);
	powerButtonController.start();
	buttonController.start();
	printerServer.setObserver(this);
	printerServer.start();
	displayTempLoop();
}

int ServiceController::displayTempLoop() {
	displayController.setInverted(false);

	while (true) {
		int64_t now = Utils::currentMillis();
		state.innerTemp = readTemp(INNER_THERMO_NAME);
		state.outerTemp = readTemp(OUTER_THERMO_NAME);
		PrintConfig config = printConfigs()[0];
		state.profileTemp = config.temperatur;
		state.profileName = config.name;

		if (turnOffTime - now > 0 && !shuttingDown) {
			displayController.drawTemperature(state.profileTemp, state.innerTemp, config.name);
		}
		else {
			displayController.turnOff();
		}

		std::ostringstream ss;
		ss << "inner: " << state.innerTemp << " outer: " << state.outerTemp << " wanted: " << state.profileTemp;
		Logger::log(ss.str());

		printerServer.setContent(state);
		fanController.tempChanged(state.innerTemp, state.profileTemp);
		Utils::sleep(1'000);
	}
	return 0;
}

int32_t ServiceController::readTemp(std::string deviceName) {
	// read file to string
	std::string path = "/sys/bus/w1/devices/" + deviceName + "/w1_slave";
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
	PrintConfigs::savePrintConfigs();
	Logger::close();
	turnOffTime = 0;
	displayController.turnOff();
}

void ServiceController::onShortPress() {
	PrintConfig config = printConfigs()[0];
	turnOffTime = Utils::currentMillis() + SCREEN_ALIVE_TIME;
	int have = readTemp(INNER_THERMO_NAME);
	displayController.drawTemperature(config.temperatur, have, config.name);
	displayController.turnOn();
}

void ServiceController::onFanStateChanged(bool isOn)
{
	if (isOn) {
		Logger::log(std::string{"fanState: on"});
	}
	else {
		Logger::log(std::string{ "fanState: off" });
	}
	displayController.setIconVisible(isOn);
}

bool ServiceController::onProfileUpdate(PrintConfig& profile)
{
	bool hasChanged = PrintConfigs::addConfig(profile);
	// update server
	state.profileName = profile.name;
	state.profileTemp = profile.temperatur;
	printerServer.setContent(state);
	return hasChanged;
}

void ServiceController::onSecondButtonClick(bool longClick)
{
	if (longClick) {
		Buzzer::singleBuzz();
		fanController.toggleControl();
	}
	else {
		// switch config
		auto configs = printConfigs();
		auto nextConfig = configs[configs.size() - 1];
		PrintConfigs::addConfig(nextConfig);
		// manually trigger display
		displayController.drawTemperature(state.profileTemp, state.innerTemp, nextConfig.name);
	}
}
