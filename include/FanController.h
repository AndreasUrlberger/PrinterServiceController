#pragma once

#include <stdint.h>  //for uint32_t

#include <functional>
#include <thread>

#include "Utils.h"

class FanController {
   public:
    inline static FanController *staticController;

   private:
    static const uint8_t fanTickPin = 7;
    uint32_t fanTicks = 0;
    uint8_t fanPin;
    static constexpr double fanRpm = 6000;
    static constexpr double fanRps = fanRpm / 60;
    static constexpr int measureInterval = 1;
    static constexpr double maxTicksInInterval = measureInterval * fanRps * 2;

    bool controlOn = true;
    bool fanOn = false;
    float temp_soll = 25;  // soll/ist Temperaturen
    float temp_abw = 1.0;  // zulaessige Temperaturabweichung +/-
    float integral = 0;    // Integral-Anteil des Reglers
    int toggle_count = 0;  // Anzahl Schaltvorgaenge

    float period = 1.f;  // Periodendauer mit der gemessen wird

    float kp = 1;                               // Proportionalfaktor
    float ki = 1;                               // Integralfaktor
    static constexpr float MAX_INTEGRATOR = 0;  // set to zero because an integrator is useless for a switch
    uint8_t ledState = false;                   // 0 -> off, 1 -> blink, 2 -> on
    std::thread blinker = std::thread(Utils::callLambda, [this]() { blinkLoop(); });
    std::function<void(bool)> onFanStateChangeHook;
    std::function<void(float)> updateFanSpeed;

    void notifyChange();
    void innerTurnOff();
    void innerTurnOn();
    void blinkLoop();

    void startFanSpeedMeasurement();
    void measureSpeed();

   public:
    FanController(uint8_t fanPin, std::function<void(bool)> onFanStateChange, std::function<void(float)> updateFanSpeed);
    void tempChanged(int32_t temp, int32_t wanted);
    void turnOff();
    void turnOn();
    void toggleControl();

    float calculateStellValue(float Regelfehler);  // berechnet die Stellgroesse aus dem Regelfehler
    void fan(float power);                         // steuert den Luefter (von 0-100)
    bool isControlOn();

    void fanTick();
};