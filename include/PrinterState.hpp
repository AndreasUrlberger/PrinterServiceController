#pragma once
#include <stdint.h>

#include <string>
#include <vector>

class PrinterState {
   public:
    class PrinterStateListener {
       public:
        virtual void onPrinterStateChanged() = 0;
    };

   private:
    // PRIVATE VARIABLES.
    bool isOn{};
    bool isTempControlActive{};
    uint64_t innerTopTemp{};
    uint64_t innerBottomTemp{};
    uint64_t outerTemp{};
    std::string profileName{};
    int32_t profileTemp{};
    float fanSpeed{};
    std::vector<PrinterStateListener*> listeners{};

   public:
    // PUBLIC FUNCTIONS.
    PrinterState() = default;
    ~PrinterState() = default;

    void addListener(PrinterStateListener* const listener);
    void notifyListeners() const;

    bool getIsOn() const;
    bool getIsTempControlActive() const;
    uint64_t getInnerTopTemp() const;
    uint64_t getInnerBottomTemp() const;
    uint64_t getOuterTemp() const;
    std::string getProfileName() const;
    int32_t getProfileTemp() const;
    float getFanSpeed() const;

    void setIsOn(const bool isOn, const bool notify = true);
    void setIsTempControlActive(const bool isTempControlActive, const bool notify = true);
    void setInnerTopTemp(const uint64_t innerTopTemp, const bool notify = true);
    void setInnerBottomTemp(const uint64_t innerBottomTemp, const bool notify = true);
    void setOuterTemp(const uint64_t outerTemp, const bool notify = true);
    void setProfileName(const std::string& profileName, const bool notify = true);
    void setProfileTemp(const int32_t profileTemp, const bool notify = true);
    void setFanSpeed(const float fanSpeed, const bool notify = true);
};
