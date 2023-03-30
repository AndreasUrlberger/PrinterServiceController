#include "FanController.hpp"

#include <math.h>
#include <pigpio.h>
#include <stdio.h>

#include <algorithm>
#include <iostream>

#include "Timing.hpp"

void FanController::onPrinterStateChanged() {
    if (state.getIsTempControlActive()) {
        turnOn();
    } else {
        turnOff();
    }
    tempChanged(state.getInnerBottomTemp(), state.getProfileTemp());
}

FanController::FanController(PrinterState& state, const FanControllerConfig& config) : state(state), config(config) {
    if (gpioInitialise() == PI_INIT_FAILED) {
        std::cerr << "ERROR: gpioInitialise failed in FanController::start\n";
        return;
    }

    int result = gpioSetMode(config.RELAY_PIN, PI_OUTPUT);
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

    result = gpioSetPullUpDown(config.RELAY_PIN, PI_PUD_DOWN);
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

    state.addListener(this);
}

void FanController::start() {
    //  Otherwise it seems to be on by default.
    turnOff();
    startFanSpeedMeasurement();

    // Start blink thread.
    std::thread([this]() { blinkLoop(); }).detach();
}

void FanController::blinkLoop() {
    gpioSetMode(config.FAN_LED_PIN, PI_OUTPUT);

    bool isLedOn = true;

    Timing::runEveryNMillis(
        config.BLINK_INTERVAL_MS, [this, &isLedOn]() {
            switch (ledState) {
                case LedState::BLINK:
                    gpioWrite(config.FAN_LED_PIN, isLedOn);
                    break;
                case LedState::ON:
                    gpioWrite(config.FAN_LED_PIN, 1);
                    break;
                case LedState::OFF:
                    gpioWrite(config.FAN_LED_PIN, 0);
                    break;
            }
            isLedOn = !isLedOn;
        });
}

// Actual and target temperature are in milli degrees celsius.
void FanController::tempChanged(int32_t actualTemperature, int32_t targetTemperature) {
    const float tempSoll = static_cast<float>(targetTemperature) / 1000;
    const float temp_ist = static_cast<float>(actualTemperature) / 1000;

    const float regelFehler = tempSoll - temp_ist;
    const float stellwert = calculateStellValue(regelFehler);

    const float fanControlValue = std::clamp(stellwert, 0.f, config.MAX_FAN_RPM);
    controlFanPWM(fanControlValue);

    const bool tempIsRight = fabs(regelFehler) <= config.maxTempDeviation;
    // Update Fan Led
    if (state.getIsTempControlActive()) {
        if (tempIsRight) {
            ledState = LedState::ON;
        } else {
            ledState = LedState::BLINK;
        }
    } else {
        ledState = LedState::OFF;
    }
}

void FanController::turnOff() {
    const int result = gpioWrite(config.RELAY_PIN, 1);
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
}

void FanController::turnOn() {
    const int result = gpioWrite(config.RELAY_PIN, 0);
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
}

void FanController::toggleControl() {
    std::cout << "Toggle Control, triggering setIsTempControlActive\n";
    state.setIsTempControlActive(!state.getIsTempControlActive(), true);
}

float FanController::calculateStellValue(const float regelfehler) {
    const float prop = regelfehler;

    integral = std::clamp(integral + regelfehler * config.TEMP_MEAS_PERIOD, config.minIntegral, config.maxIntegral);
    const float stellwert = -config.kp * prop - config.ki * integral;

    return fmax(0.f, stellwert);
}

void FanController::controlFanPWM(const float power) {
    const int controlPower = round(power / config.MAX_FAN_RPM * FAN_PWM_RANGE);
    // Enable hardware PWM on FAN_PWM_PIN.
    int result = gpioHardwarePWM(config.FAN_PWM_PIN, FAN_PWM_FREQUENCY, controlPower);
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
        return;
    }

    // Read hardware PWM frequency
    result = gpioGetPWMfrequency(config.FAN_PWM_PIN);
    //  Returns the frequency (in hertz) used for the GPIO if OK, otherwise PI_BAD_USER_GPIO.
    if (result == PI_BAD_USER_GPIO) {
        std::cerr << "ERROR: Get Frequency, Fan PWM pin is not a valid GPIO pin.\n";
    }
}

void FanController::fanTick() {
    fanTicks++;
}

void FanController::measureSpeed() {
    // Not thread safe but really doesnt really matter for this use case, rough estimate is enough.
    const uint32_t measuredTicks = fanTicks;
    fanTicks = UINT32_C(0);

    const float fanSpeed = measuredTicks / MAX_TICKS_IN_INTERVAL;
    // Do not notify listeners since nobody is interested in the fan speed.
    state.setFanSpeed(fanSpeed, false);
}

void FanController::startFanSpeedMeasurement() {
    gpioSetMode(config.FAN_TICK_PIN, PI_INPUT);
    gpioSetPullUpDown(config.FAN_TICK_PIN, PI_PUD_DOWN);
    gpioSetISRFuncEx(config.FAN_TICK_PIN, EITHER_EDGE, 0, interruptHandler, this);

    std::thread([this]() {
        Timing::runEveryNMillis(config.FAN_SPEED_MEAS_PERIOD_MS, [this]() {
            measureSpeed();
        });
    }).detach();
}

void FanController::interruptHandler(const int gpio, const int level, const uint32_t tick, void* fanController) {
    if (fanController != nullptr) {
        FanController* const controller = static_cast<FanController*>(fanController);
        controller->fanTick();
    }
}
