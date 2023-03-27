#include "FanController.h"

#include <math.h>
#include <pigpio.h>
#include <stdio.h>
#include <wiringPi.h>

#include <algorithm>
#include <iostream>

FanController::FanController(std::function<void(bool)> onFanStateChange, std::function<void(float)> updateFanSpeed) {
    onFanStateChangeHook = onFanStateChange;
    this->updateFanSpeed = updateFanSpeed;
    wiringPiSetupSys();
    gpioInitialise();
    pinMode(RELAY_PIN, OUTPUT);
    // Otherwise it seems to be on by default.
    turnOff();
    // digitalWrite(FAN_LED_PIN, false);

    startFanSpeedMeasurement();
}

void FanController::notifyChange() {
    onFanStateChangeHook(controlOn);
}

void FanController::blinkLoop() {
    wiringPiSetupSys();  // just in case
    pinMode(FAN_LED_PIN, OUTPUT);

    bool isLedOn = true;
    while (true) {
        switch (ledState) {
            case 1:
                digitalWrite(FAN_LED_PIN, isLedOn);
                break;
            case 2:
                digitalWrite(FAN_LED_PIN, 1);
                break;
            default:
                digitalWrite(FAN_LED_PIN, 0);
        }
        isLedOn = !isLedOn;
        Utils::sleep(500);
    }
}

void FanController::tempChanged(int32_t actualTemperature, int32_t targetTemperature) {
    tempSoll = static_cast<float>(targetTemperature) / 1000;
    float temp_ist = static_cast<float>(actualTemperature) / 1000;

    const float regelFehler = tempSoll - temp_ist;
    const float stellwert = calculateStellValue(regelFehler);

    const float fanControlValue = std::clamp(stellwert, 0.f, MAX_FAN_RPM);
    controlFanPWM(fanControlValue);

    const bool tempIsRight = fabs(regelFehler) <= tempAbw;
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
    digitalWrite(RELAY_PIN, HIGH);
    notifyChange();
}

void FanController::turnOn() {
    digitalWrite(RELAY_PIN, LOW);
    notifyChange();
}

void FanController::toggleControl() {
    controlOn = !controlOn;

    if (controlOn) {
        turnOn();
    } else {
        turnOff();
    }
}

float FanController::calculateStellValue(float regelfehler) {
    const float prop = regelfehler;

    integral = std::clamp(integral + regelfehler * TEMP_MEAS_PERIOD, MIN_INTEGRAL, MAX_INTEGRAL);
    const float stellwert = -kp * prop - ki * integral;

    return fmax(0.f, stellwert);
}

void FanController::controlFanPWM(float power) {
    const int controlPower = round(power / MAX_FAN_RPM * 255);
    // Max control power is 255.
    const int result = gpioPWM(FAN_PWM_PIN, controlPower);
    if (result != 0) {
        // Something went wrong.
        switch (result) {
            case PI_BAD_USER_GPIO:
                std::cerr << "ERROR: Fan PWM pin is not a valid GPIO pin.\n";
                break;
            case PI_BAD_DUTYCYCLE:
                std::cerr << "ERROR: Fan PWM duty cycle is not in range 0-255.\n";
                break;
            default:
                std::cerr << "ERROR: Fan PWM control failed with unknown error code: " << result << "\n";
                break;
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

    const float fanSpeed = measuredTicks / MAX_TICKS_IN_INTERVAL;
    updateFanSpeed(fanSpeed);
}

void FanController::startFanSpeedMeasurement() {
    staticController = this;

    wiringPiSetupSys();

    pinMode(FAN_TICK_PIN, INPUT);
    pullUpDnControl(FAN_TICK_PIN, PUD_DOWN);
    wiringPiISR(FAN_TICK_PIN, INT_EDGE_BOTH, fanTickWrapper);

    std::thread([this]() {
        auto start = std::chrono::steady_clock::now();
        auto chronoInterval = std::chrono::seconds(FAN_SPEED_MEAS_PERIOD);
        auto targetTime = start + chronoInterval;
        while (true) {
            measureSpeed();
            std::this_thread::sleep_until(targetTime);
            targetTime += chronoInterval;
        }
    }).detach();
}
