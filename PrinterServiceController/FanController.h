#pragma once

#include <stdint.h> //for uint32_t
#include <thread>
#include <functional>
#include "Utils.h"

class FanObserver {
public:
	virtual void onFanStateChanged(bool isOn) = 0;
};

class FanController
{
private:
	uint8_t fanPin;
	bool controlOn = true;
	bool fanOn = false;
	float temp_soll = 25;					// soll/ist Temperaturen
	float temp_abw = 1.0; 					// zulaessige Temperaturabweichung +/-
	float integral = 0;						// Integral-Anteil des Reglers
	int toggle_count = 0;					// Anzahl Schaltvorgaenge

	float period = 1.f; 					// Periodendauer mit der gemessen wird

	float kp = 1;							// Proportionalfaktor
	float ki = 1;							// Integralfaktor
	static constexpr float MAX_INTEGRATOR = 0; // set to zero because an integrator is useless for a switch
	FanObserver* observer = nullptr;
	uint8_t ledState = false; // 0 -> off, 1 -> blink, 2 -> on
	std::thread blinker = std::thread(Utils::callLambda, [this]() {blinkLoop(); });

	void notifyObserver();
	void innerTurnOff();
	void innerTurnOn();
	void blinkLoop();


public:
	FanController(uint8_t fanPin);
	void tempChanged(int32_t temp, int32_t wanted);
	void turnOff();
	void turnOn();
	void toggleControl();

	float calculateStellValue(float Regelfehler);	// berechnet die Stellgroesse aus dem Regelfehler
	void fan(float power);				// steuert den Luefter (von 0-100)
	void setObserver(FanObserver* observer);
	bool isControlOn();

};