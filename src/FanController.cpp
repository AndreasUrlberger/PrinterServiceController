#include "FanController.hpp"

#include <math.h>
#include <pigpio.h>
#include <stdio.h>

#include <algorithm>
#include <iostream>

#include "Timing.hpp"

FanController::FanController(std::function<void(bool)> onFanStateChange, std::function<void(float)> updateFanSpeed) {
    onFanStateChangeHook = onFanStateChange;
    this->updateFanSpeed = updateFanSpeed;
    if (gpioInitialise() == PI_INIT_FAILED) {
        std::cerr << "ERROR: gpioInitialise failed in FanController::start\n";
        return;
    }

    int result = gpioSetMode(RELAY_PIN, PI_OUTPUT);
    // 0 if OK, otherwise PI_BAD_GPIO or PI_BAD_MODE.
    if (result != 0) {
        // Something went wrong.
        switch (result) {
            case PI_BAD_GPIO:
                std::cerr << "ERROR: Setting Mode, Relay pin is not a valid GPIO pin.\n";
                break;
            case PI_BAD_MODE:
                std::cerr << "ERROR: Setting Mode, Relay pin is not a valid GPIO output pin.\n";
                break;
            default:
                std::cerr << "ERROR: Setting Mode, Relay pin setup failed with unknown error code: " << result << "\n";
                break;
        }
    }

    result = gpioSetPullUpDown(RELAY_PIN, PI_PUD_DOWN);
    // 0 if OK, otherwise PI_BAD_GPIO or PI_BAD_MODE.
    if (result != 0) {
        // Something went wrong.
        switch (result) {
            case PI_BAD_GPIO:
                std::cerr << "ERROR: Setting Pull Up/Down, Relay pin is not a valid GPIO pin.\n";
                break;
            case PI_BAD_MODE:
                std::cerr << "ERROR: Setting Pull Up/Down, Relay pin is not a valid GPIO output pin.\n";
                break;
            default:
                std::cerr << "ERROR: Setting Pull Up/Down, Relay pin setup failed with unknown error code: " << result << "\n";
                break;
        }
    }
}

void FanController::start() {
    //  Otherwise it seems to be on by default.
    turnOff();
    startFanSpeedMeasurement();

    // Start blink thread.
    std::thread([this]() { blinkLoop(); }).detach();
}

void FanController::notifyChange() {
    onFanStateChangeHook(controlOn);
}

void FanController::blinkLoop() {
    gpioSetMode(FAN_LED_PIN, PI_OUTPUT);

    bool isLedOn = true;
    while (true) {
        switch (ledState) {
            case 1:
                gpioWrite(FAN_LED_PIN, isLedOn);
                break;
            case 2:
                gpioWrite(FAN_LED_PIN, 1);
                break;
            default:
                gpioWrite(FAN_LED_PIN, 0);
        }
        isLedOn = !isLedOn;
        Timing::sleepMillis(500);
    }
}

// Actual and target temperature are in milli degrees celsius.
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
    int result = gpioWrite(RELAY_PIN, 1);
    // 0 if OK, otherwise PI_BAD_GPIO or PI_BAD_MODE.
    if (result != 0) {
        // Something went wrong.
        switch (result) {
            case PI_BAD_GPIO:
                std::cerr << "ERROR: Turning off, Relay pin is not a valid GPIO pin.\n";
                break;
            case PI_BAD_MODE:
                std::cerr << "ERROR: Turning off, Relay pin is not a valid GPIO output pin.\n";
                break;
            default:
                std::cerr << "ERROR: Turning off, Relay pin setup failed with unknown error code: " << result << "\n";
                break;
        }
    }
    notifyChange();
}

void FanController::turnOn() {
    int result = gpioWrite(RELAY_PIN, 0);
    // 0 if OK, otherwise PI_BAD_GPIO or PI_BAD_MODE.
    if (result != 0) {
        // Something went wrong.
        switch (result) {
            case PI_BAD_GPIO:
                std::cerr << "ERROR: Turning on, Relay pin is not a valid GPIO pin.\n";
                break;
            case PI_BAD_MODE:
                std::cerr << "ERROR: Turning on, Relay pin is not a valid GPIO output pin.\n";
                break;
            default:
                std::cerr << "ERROR: Turning on, Relay pin setup failed with unknown error code: " << result << "\n";
                break;
        }
    }
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
    const int controlPower = round(power / MAX_FAN_RPM * FAN_PWM_RANGE);
    // Enable hardware PWM on FAN_PWM_PIN.
    int result = gpioHardwarePWM(FAN_PWM_PIN, FAN_PWM_FREQUENCY, controlPower);
    // Returns 0 if OK, otherwise PI_BAD_GPIO, PI_NOT_HPWM_GPIO, PI_BAD_HPWM_DUTY, PI_BAD_HPWM_FREQ, or PI_HPWM_ILLEGAL.
    if (result != 0) {
        // Something went wrong.
        switch (result) {
            case PI_BAD_GPIO:
                std::cerr << "ERROR: Fan PWM pin is not a valid GPIO pin.\n";
                break;
            case PI_NOT_HPWM_GPIO:
                std::cerr << "ERROR: Fan PWM pin is not a valid hardware PWM pin.\n";
                break;
            case PI_BAD_HPWM_DUTY:
                std::cerr << "ERROR: Fan PWM duty cycle is not in valid range.\n";
                break;
            case PI_BAD_HPWM_FREQ:
                std::cerr << "ERROR: Fan PWM frequency is not in valid range.\n";
                break;
            case PI_HPWM_ILLEGAL:
                std::cerr << "ERROR: Fan PWM frequency is not in valid range.\n";
                break;
            default:
                std::cerr << "ERROR: Fan PWM pin setup failed with unknown error code: " << result << "\n";
                break;
        }
    }

    // Read hardware PWM frequency
    result = gpioGetPWMfrequency(FAN_PWM_PIN);
    //  Returns the frequency (in hertz) used for the GPIO if OK, otherwise PI_BAD_USER_GPIO.
    if (result == PI_BAD_USER_GPIO) {
        std::cerr << "ERROR: Get Frequency, Fan PWM pin is not a valid GPIO pin.\n";
    }
}

bool FanController::isControlOn() {
    return controlOn;
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
    gpioSetMode(FAN_TICK_PIN, PI_INPUT);
    gpioSetPullUpDown(FAN_TICK_PIN, PI_PUD_DOWN);
    gpioSetISRFuncEx(FAN_TICK_PIN, EITHER_EDGE, 0, interruptHandler, this);

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

void FanController::interruptHandler(int gpio, int level, uint32_t tick, void* fanController) {
    if (fanController != nullptr) {
        FanController* const controller = static_cast<FanController*>(fanController);
        controller->fanTick();
    }
}
