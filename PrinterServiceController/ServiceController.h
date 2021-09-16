#pragma once

#include "DisplayController.h"
#include "PowerButtonController.h"
#include "FanController.h"
#include "PrinterServer.h"
#include "Utils.h"
#include "PrintConfigs.h"
#include "ButtonController.h"

static constexpr auto INNER_THERMO_NAME = "28-2ca0a72153ff";
static constexpr auto OUTER_THERMO_NAME = "28-baa0a72915ff";

class ServiceController {
private:
	PowerButtonController powerButtonController{
		[this]() {onShutdown(); },
		[this]() {onShortPress(); }
	};
	DisplayController displayController;
	FanController fanController{ 
		6, 
		[this](bool state) {onFanStateChanged(state); } 
	};
	PrinterServer printerServer{ 
		[this]() {onShutdown(); },
		[this](PrintConfig& config) {return onProfileUpdate(config); } 
	};
	ButtonController buttonController{ 
		[this](bool longClick) {onSecondButtonClick(longClick);} 
	};
	static constexpr uint64_t SCREEN_ALIVE_TIME = 5'000;
	int64_t turnOffTime = Utils::currentMillis() + SCREEN_ALIVE_TIME;
	bool shuttingDown = false;
	int displayTempLoop();
	int32_t readTemp(std::string deviceName);
	void onShutdown();
	void onShortPress();
	void onFanStateChanged(bool state);
	bool onProfileUpdate(PrintConfig& profile);
	void onSecondButtonClick(bool longClick);
	PrinterState state;

public:
	void run();
};

