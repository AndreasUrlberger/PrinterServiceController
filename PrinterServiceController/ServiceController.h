#pragma once

#include "DisplayController.h"
#include "PowerButtonController.h"
#include "FanController.h"
#include "PrinterServer.h"
#include "Utils.h"
#include "PrintConfigs.h"

static constexpr auto INNER_THERMO_NAME = "28-2ca0a72153ff";
static constexpr auto OUTER_THERMO_NAME = "28-baa0a72915ff";

class ServiceController : public PowerButtonObserver, public FanObserver, public PServerObserver {
private:
	PowerButtonController powerButtonController;
	DisplayController displayController;
	FanController fanController{ 6 };
	PrinterServer printerServer;
	static constexpr uint64_t SCREEN_ALIVE_TIME = 30'000;
	int64_t turnOffTime = Utils::currentMillis() + SCREEN_ALIVE_TIME;
	bool shuttingDown = false;
	int displayTempLoop();
	int32_t readTemp(std::string deviceName);
	virtual void onShutdown() override;
	virtual void onShortPress() override;
	virtual void onFanStateChanged(bool state) override;
	virtual bool onProfileUpdate(PrintConfig& profile) override;
	PrinterState state;

public:
	void run();
};

