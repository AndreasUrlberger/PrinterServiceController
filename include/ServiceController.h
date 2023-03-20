#pragma once

#include "DisplayController.h"
#include "PowerButtonController.h"
#include "FanController.h"
#include "PrinterServer.h"
#include "WsProtoHandler.h"
#include "Utils.h"
#include "PrintConfigs.h"
#include "ButtonController.h"

static constexpr auto INNER_THERMO_NAME = "28-2ca0a72153ff";
static constexpr auto OUTER_THERMO_NAME = "28-baa0a72915ff";

class ServiceController
{
private:
	static constexpr uint64_t SCREEN_ALIVE_TIME = 0'000;
	int64_t turnOffTime = Utils::currentMillis() + SCREEN_ALIVE_TIME;
	bool shuttingDown = false;
	int displayTempLoop();
	void updateDisplay();
	int32_t readTemp(std::string deviceName);
	void onShutdown();
	void onShortPress();
	void onFanStateChanged(bool state);
	bool onProfileUpdate(PrintConfig &profile);
	void onSecondButtonClick(bool longClick);
	void onChangeFanControl(bool isOn);
	PrinterState state;

	PowerButtonController powerButtonController{
		[this]()
		{ onShutdown(); },
		[this]()
		{ onShortPress(); }};
	DisplayController displayController;
	FanController fanController{
		6,
		[this](bool state)
		{ onFanStateChanged(state); }};
	WsProtoHandler printerServer{
		[this]()
		{ onShutdown(); },
		[this](PrintConfig &config)
		{ return onProfileUpdate(config); },
		[this](bool isOn)
		{ onChangeFanControl(isOn); }};
	ButtonController buttonController{
		[this](bool longClick)
		{ onSecondButtonClick(longClick); }};

public:
	void run();
};
