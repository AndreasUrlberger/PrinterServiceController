#include "Buzzer.hpp"

#include <pigpio.h>
#include <stdint.h>

#include <iostream>

#include "Timing.hpp"

Buzzer::Buzzer(const uint8_t pin) : pin(pin) {}

void Buzzer::singleBuzz() {
    setup();
    gpioWrite(pin, 1);
    Timing::sleepMillis(333);
    gpioWrite(pin, 0);
}

void Buzzer::doubleBuzz() {
    setup();
    gpioWrite(pin, 1);
    Timing::sleepMillis(80);
    gpioWrite(pin, 0);
    Timing::sleepMillis(30);
    gpioWrite(pin, 1);
    Timing::sleepMillis(80);
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
