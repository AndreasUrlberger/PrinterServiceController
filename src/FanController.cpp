#include "FanController.h"

#include <math.h>
#include <stdio.h>
#include <wiringPi.h>

#include <algorithm>
#include <iostream>

constexpr int FAN_LED = 21;

FanController::FanController(uint8_t fanPin, std::function<void(bool)> onFanStateChange, std::function<void(float)> updateFanSpeed) {
    onFanStateChangeHook = onFanStateChange;
    this->updateFanSpeed = updateFanSpeed;
    this->fanPin = fanPin;
    wiringPiSetupSys();
    pinMode(fanPin, OUTPUT);
    turnOff();  // Otherwise it seems to be on by default.
                // digitalWrite(FAN_LED, false);

    startFanSpeedMeasurement();
}

void FanController::notifyChange() {
    onFanStateChangeHook(fanOn);
}

void FanController::innerTurnOff() {
    if (controlOn) {
        turnOff();
    }
}

void FanController::innerTurnOn() {
    if (controlOn) {
        turnOn();
    }
}

void FanController::blinkLoop() {
    wiringPiSetupSys();  // just in case
    pinMode(FAN_LED, OUTPUT);

    bool isLedOn = true;
    while (true) {
        switch (ledState) {
            case 1:
                digitalWrite(FAN_LED, isLedOn);
                break;
            case 2:
                digitalWrite(FAN_LED, 1);
                break;
            default:
                digitalWrite(FAN_LED, 0);
        }
        isLedOn = !isLedOn;
        Utils::sleep(500);
    }
}

void FanController::tempChanged(int32_t temp, int32_t wanted) {
    temp_soll = static_cast<float>(wanted) / 1000;
    float temp_ist = static_cast<float>(temp) / 1000;

    float regelf = temp_soll - temp_ist;
    float stell = calculateStellValue(regelf);

    bool tempIsRight = fabs(regelf) <= temp_abw;
    if (!tempIsRight) {
        fan(stell);
    } else {
        // printf("keine Aktion \n");
    }

    // printf("aktuelle Temperatur: %.1fC, Stellgroesse= %.2f, Integral: %.2f \n", temp_ist, stell, integral);

    // update Fan Led
    if (controlOn) {
        if (tempIsRight) {
            // leuchten
            ledState = 2;
        } else {
            // blinken
            ledState = 1;
        }
    } else {
        // licht aus
        ledState = 0;
    }
}

void FanController::turnOff() {
    digitalWrite(fanPin, HIGH);
    fanOn = false;
    notifyChange();
}

void FanController::turnOn() {
    digitalWrite(fanPin, LOW);
    fanOn = true;
    notifyChange();
}

void FanController::toggleControl() {
    controlOn = !controlOn;
    if (!controlOn && fanOn) {
        turnOff();
    }
}

float FanController::calculateStellValue(float Regelfehler) {
    float prop = Regelfehler;

    integral = std::clamp(integral + Regelfehler * period, -MAX_INTEGRATOR, MAX_INTEGRATOR);
    float stell = -kp * prop - ki * integral;

    return fmax(0.f, stell);
}

void FanController::fan(float power)  // Luefter an/aus
{
    if (power > 0) {
        if (fanOn) {
            // printf("Luefter immernoch an \n");
        } else {
            // printf("Luefter an \n"); // Luefter anschalten
            innerTurnOn();
            toggle_count++;
            // printf("Anzahl Schaltvorgaenge: %d \n", toggle_count);
        }
    } else {
        if (fanOn) {
            // printf("Luefter aus \n"); // Luefter ausschalten
            innerTurnOff();
        } else {
            // printf("Luefter immer noch aus \n"); // Luefter ausschalten
        }
    }
}

bool FanController::isControlOn() {
    return controlOn;
}

void fanTickWrapper() {
    if (FanController::staticController != nullptr) {
        FanController::staticController->fanTick();
    }
}

void FanController::fanTick() {
    fanTicks++;
}

void FanController::measureSpeed() {
    // Not thread safe but really doesnt matter for this use case.
    uint32_t measuredTicks = fanTicks;
    fanTicks = 0;

    const double fanSpeed = measuredTicks / maxTicksInInterval;
    updateFanSpeed(fanSpeed);
}

void FanController::startFanSpeedMeasurement() {
    staticController = this;

    wiringPiSetupSys();

    pinMode(fanTickPin, INPUT);
    pullUpDnControl(fanTickPin, PUD_DOWN);
    wiringPiISR(fanTickPin, INT_EDGE_BOTH, fanTickWrapper);

    std::thread([this]() {
        auto start = std::chrono::steady_clock::now();
        auto chronoInterval = std::chrono::seconds(measureInterval);
        auto targetTime = start + chronoInterval;
        while (true) {
            measureSpeed();
            std::this_thread::sleep_until(targetTime);
            targetTime += chronoInterval;
        }
    }).detach();
}
