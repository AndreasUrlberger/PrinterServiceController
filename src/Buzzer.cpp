#include "Buzzer.h"

#include <pigpio.h>
#include <stdint.h>

#include <iostream>

#include "Utils.h"

static constexpr int pin = 26;

void Buzzer::singleBuzz() {
    setup();
    gpioWrite(pin, 1);
    Utils::sleep(333);
    gpioWrite(pin, 0);
}

void Buzzer::doubleBuzz() {
    setup();
    gpioWrite(pin, 1);
    Utils::sleep(80);
    gpioWrite(pin, 0);
    Utils::sleep(30);
    gpioWrite(pin, 1);
    Utils::sleep(80);
    gpioWrite(pin, 0);
}

void Buzzer::setup() {
    if (!isSetup) {
        if (gpioInitialise() == PI_INIT_FAILED) {
            std::cerr << "ERROR: gpioInitialise failed\n";
            return;
        } else {
            isSetup = true;
            gpioSetMode(pin, PI_OUTPUT);
        }
    }
}
