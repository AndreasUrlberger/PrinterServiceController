#pragma once

#include <stdint.h>  //for uint32_t

#include <functional>
#include <thread>

#include "Utils.h"

class FanController {
    // Variables.
   private:
    // On/Off control.
    static constexpr uint8_t RELAY_PIN = 11;
    static constexpr uint8_t FAN_LED_PIN = 23;

    // PWM control.
    static constexpr uint8_t FAN_PWM_PIN = 12;
    static constexpr uint32_t FAN_PWM_FREQUENCY = 25'000;
    static constexpr uint32_t FAN_PWM_RANGE = 1'000'000;
    static constexpr uint8_t FAN_TICK_PIN = 7;

    // Fan speed measurement.
    static constexpr float MIN_FAN_RPM = 750;
    static constexpr float MAX_FAN_RPM = 3000;
    static constexpr float FAN_RPS = MAX_FAN_RPM / 60.0;
    static constexpr int FAN_SPEED_MEAS_PERIOD = 1;
    static constexpr float MAX_TICKS_IN_INTERVAL = FAN_SPEED_MEAS_PERIOD * FAN_RPS * 2;
    uint32_t fanTicks = 0;

    // Temperature control.
    static constexpr float TEMP_MEAS_PERIOD = 1.f;
    static constexpr float MIN_INTEGRAL = -5000;
    static constexpr float MAX_INTEGRAL = 5000;
    float tempSoll = 25;  // soll/ist Temperaturen
    float tempAbw = 1.0;  // zulaessige Temperaturabweichung +/-
    float integral = 0;   // Integral-Anteil des Reglers
    int toggleCount = 0;  // Anzahl Schaltvorgaenge
    float kp = 10;        // Proportionalfaktor
    float ki = 10;        // Integralfaktor
    bool controlOn = false;

    // Other.
    uint8_t ledState = false;  // 0 -> off, 1 -> blink, 2 -> on
    std::function<void(bool)> onFanStateChangeHook;
    std::function<void(float)> updateFanSpeed;

    // Functions.
   public:
    FanController(std::function<void(bool)> onFanStateChange, std::function<void(float)> updateFanSpeed);

    static void interruptHandler(int gpio, int level, uint32_t tick, void* buttonController);

    void tempChanged(int32_t temp, int32_t wanted);
    void turnOff();
    void turnOn();
    void toggleControl();

    float calculateStellValue(float Regelfehler);  // berechnet die Stellgroesse aus dem Regelfehler
    void controlFanPWM(float power);
    bool isControlOn();
    void fanTick();
    void start();

   private:
    void notifyChange();
    void blinkLoop();

    void startFanSpeedMeasurement();
    void measureSpeed();
};