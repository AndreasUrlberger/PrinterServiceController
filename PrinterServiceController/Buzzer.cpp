#include "Buzzer.h"
#include <wiringPi.h>
#include <stdint.h>

static constexpr int pin = 19;

void Buzzer::singleBuzz() {
	setup();
	digitalWrite(pin, HIGH);
	delay(333);
	digitalWrite(pin, LOW);
}

void Buzzer::doubleBuzz() {
	setup();
	digitalWrite(pin, HIGH);
	delay(80);
	digitalWrite(pin, LOW);
	delay(30);
	digitalWrite(pin, HIGH);
	delay(80);
	digitalWrite(pin, LOW);
}

void Buzzer::setup()
{
	if (!isSetup) {
		isSetup = true;
		wiringPiSetupSys();
		pinMode(pin, OUTPUT);
	}
}

