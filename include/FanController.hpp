#pragma once

#include <stdint.h>

#include <functional>
#include <thread>

#include "PrinterState.hpp"

class FanController : private PrinterState::PrinterStateListener {
   public:
    struct FanControllerConfig {
        // PWM control.
        const uint8_t FAN_PWM_PIN{};
        const uint8_t FAN_TICK_PIN{};

        // On/Off control.
        const uint8_t RELAY_PIN{};
        const uint8_t FAN_LED_PIN{};

        // Fan speed measurement.
        const float MIN_FAN_RPM{};
        const float MAX_FAN_RPM{};
        const uint64_t FAN_SPEED_MEAS_PERIOD_MS{};

        // Temperature control.
        const float TEMP_MEAS_PERIOD{};
        float minIntegral{};
        float maxIntegral{};
        float maxTempDeviation{};
        float kp{10};  // Proportionalfaktor
        float ki{10};  // Integralfaktor

        // Other.
        const uint64_t BLINK_INTERVAL_MS{};
    };

   private:
    enum LedState {
        OFF,
        BLINK,
        ON
    };

    // PRIVATE VARIABLES.

    // Other.
    LedState ledState{LedState::OFF};
    PrinterState& state;
    const FanControllerConfig config;

    // PWM control.
    static constexpr uint32_t FAN_PWM_FREQUENCY = 25'000;
    static constexpr uint32_t FAN_PWM_RANGE = 1'000'000;

    // Fan speed measurement.
    const float FAN_RPS = config.MAX_FAN_RPM / 60.0;
    const float MAX_TICKS_IN_INTERVAL = (config.FAN_SPEED_MEAS_PERIOD_MS / 1000.0f) * FAN_RPS * 2;
    uint32_t fanTicks = 0;

    // Temperature control.
    float integral{0};  // Integral-Anteil des Reglers

    // PRIVATE FUNCTIONS.
   private:
    void fanTick();
    float calculateStellValue(const float Regelfehler);  // berechnet die Stellgroesse aus dem Regelfehler
    void controlFanPWM(const float power);
    void blinkLoop();
    void startFanSpeedMeasurement();
    void measureSpeed();
    void tempChanged(const int32_t temp, const int32_t wanted);
    static void interruptHandler(const int gpio, const int level, const uint32_t tick, void* buttonController);

    // PUBLIC FUNCTIONS.
   public:
    FanController(PrinterState& state, const FanControllerConfig& config);
    void onPrinterStateChanged() override;

    void turnOff();
    void turnOn();
    void toggleControl();

    void start();
};