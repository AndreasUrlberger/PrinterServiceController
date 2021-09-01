#include "FanController.h"
#include <wiringPi.h>
#include <iostream>

#include <stdio.h>
#include <math.h>
#include <algorithm>

FanController::FanController(uint8_t fanPin)
{
	this->fanPin = fanPin;
	wiringPiSetupSys();
	pinMode(fanPin, OUTPUT);
	turnOff(); // Otherwise it seems to be on by default
}

void FanController::notifyObserver()
{
	if (observer != nullptr) {
		observer->onFanStateChanged(fanOn);
	}
}

void FanController::tempChanged(int32_t temp, int32_t wanted) {
	temp_soll = static_cast<float>(wanted);
	float temp_ist = static_cast<float>(temp) / 1000;
	//std::cout << "tempChanged: " << temp_ist << std::endl;
	
	float regelf = temp_soll - temp_ist;
	float stell = calculateStellValue(regelf);

	if (fabs(regelf) > temp_abw)
	{
		fan(stell);
	}
	else
	{
		//printf("keine Aktion \n");
	}

	//printf("aktuelle Temperatur: %.1fC, Stellgroesse= %.2f, Integral: %.2f \n", temp_ist, stell, integral);
}

void FanController::turnOff() {
	digitalWrite(fanPin, HIGH);
	fanOn = false;
	notifyObserver();
}

void FanController::turnOn() {
	digitalWrite(fanPin, LOW);
	fanOn = true;
	notifyObserver();
}

float FanController::calculateStellValue(float Regelfehler)
{
	float prop = Regelfehler;
	
	integral = std::clamp(integral + Regelfehler * period, -MAX_INTEGRATOR, MAX_INTEGRATOR);
	float stell = -kp * prop - ki * integral;

	return fmax(0.f, stell);
}

void FanController::fan(float power) //Luefter an/aus
{
	if (power > 0)
	{
		if (fanOn)
		{
			//printf("Luefter immernoch an \n");
		}
		else
		{
			//printf("Luefter an \n"); // Luefter anschalten
			turnOn();
			toggle_count++;
			//printf("Anzahl Schaltvorgaenge: %d \n", toggle_count);
		}
	}
	else
	{
		if (fanOn) {
			//printf("Luefter aus \n"); // Luefter ausschalten
			turnOff();
		}
		else {
			//printf("Luefter immer noch aus \n"); // Luefter ausschalten
		}
	}
}

void FanController::setObserver(FanObserver* observer)
{
	this->observer = observer;
}
