#pragma once

class Buzzer {
   private:
    bool isSetup = false;
    const uint8_t pin;

    void setup();

   public:
    Buzzer(const uint8_t pin);
    ~Buzzer() = default;
    void singleBuzz();
    void doubleBuzz();
};
