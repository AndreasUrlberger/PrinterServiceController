#pragma once

#include "DisplayController.h"
#include "PowerButtonController.h"
#include "FanController.h"
#include "PrinterServer.h"
#include "Utils.h"

class ServiceController : public PowerButtonObserver, public FanObserver{
private: 
	PowerButtonController powerButtonController;
	DisplayController displayController;
	FanController fanController{ 6 };
	PrinterServer printerServer;
	static constexpr uint64_t SCREEN_ALIVE_TIME = 30'000;
	int64_t turnOffTime = Utils::currentMillis() + SCREEN_ALIVE_TIME;
	bool shuttingDown = false;
	int displayTempLoop();
	int32_t readTemp();
	virtual void onShutdown() override;
	virtual void onShortPress() override;
	virtual void onFanStateChanged(bool state) override;

public:
	void run();
};

