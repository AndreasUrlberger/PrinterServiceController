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
	state.state = true;
	state.tempControl = fanController.isControlOn();

	powerButtonController.start();
	buttonController.start();
	printerServer.start();
	displayTempLoop();
}

int ServiceController::displayTempLoop()
{
	displayController.setInverted(false);

	while (true)
	{
		state.innerTemp = readTemp(INNER_THERMO_NAME);
		// state.outerTemp = readTemp(OUTER_THERMO_NAME);
		state.outerTemp = 0;
		updateDisplay();

		std::ostringstream ss;
		ss << "inner: " << state.innerTemp << " outer: " << state.outerTemp << " wanted: " << state.profileTemp;
		Logger::log(ss.str());

		printerServer.updateState(state);
		fanController.tempChanged(state.innerTemp, state.profileTemp);
		Utils::sleep(100);
	}
	return 0;
}

void ServiceController::updateDisplay()
{
	PrintConfig config = printConfigs()[0];
	state.profileTemp = config.temperature;
	state.profileName = config.name;
	int64_t now = Utils::currentMillis();
	if (turnOffTime - now > 0 && !shuttingDown)
	{
		displayController.drawTemperature(state.profileTemp, state.innerTemp, config.name);
	}
	else
	{
		displayController.turnOff();
	}
}

int32_t ServiceController::readTemp(std::string deviceName)
{
	// read file to string
	std::string path = "/sys/bus/w1/devices/" + deviceName + "/w1_slave";
	auto stream = std::ifstream{path.data()};
	std::ostringstream sstr;
	sstr << stream.rdbuf();
	stream.close();
	std::string input = sstr.str();

	// find temp
	const size_t index = input.find("t=") + 2; // start of temperature
	return index < input.length() ? stoi(input.substr(index)) : 0;
}

void ServiceController::onShutdown()
{
	shuttingDown = true;
	PrintConfigs::savePrintConfigs();
	Logger::close();
	turnOffTime = 0;
	displayController.turnOff();
	system("sudo shutdown -h now");
}

void ServiceController::onShortPress()
{
	turnOffTime = Utils::currentMillis() + SCREEN_ALIVE_TIME;
	updateDisplay();
	displayController.turnOn();
}

void ServiceController::onFanStateChanged(bool isOn)
{
	if (isOn)
	{
		Logger::log(std::string{"fanState: on"});
	}
	else
	{
		Logger::log(std::string{"fanState: off"});
	}
	displayController.setIconVisible(isOn);
}

bool ServiceController::onProfileUpdate(PrintConfig &profile)
{
	bool hasChanged = PrintConfigs::addConfig(profile);
	// update server
	state.profileName = profile.name;
	state.profileTemp = profile.temperature;
	printerServer.updateState(state);
	return hasChanged;
}

void ServiceController::onSecondButtonClick(bool longClick)
{
	turnOffTime = Utils::currentMillis() + SCREEN_ALIVE_TIME;
	if (longClick)
	{
		Buzzer::singleBuzz();
		fanController.toggleControl();
	}
	else
	{
		// switch config
		auto configs = printConfigs();
		auto nextConfig = configs[configs.size() - 1];
		PrintConfigs::addConfig(nextConfig);
		// manually trigger display
		updateDisplay();
	}
	displayController.turnOn();
}

void ServiceController::onChangeFanControl(bool isOn)
{
	if (fanController.isControlOn() xor isOn)
	{
		fanController.toggleControl();
		state.tempControl = fanController.isControlOn();
		printerServer.updateState(state);
	}
}
