#include "PrinterState.hpp"

#include <iostream>

void PrinterState::addListener(PrinterStateListener* const listener) {
    listeners.push_back(listener);
}

void PrinterState::notifyListeners() const {
    for (PrinterStateListener* const listener : listeners) {
        listener->onPrinterStateChanged();
    }
}

bool PrinterState::getIsOn() const {
    return isOn;
}

bool PrinterState::getIsTempControlActive() const {
    return isTempControlActive;
}

uint64_t PrinterState::getInnerTopTemp() const {
    return innerTopTemp;
}

uint64_t PrinterState::getInnerBottomTemp() const {
    return innerBottomTemp;
}

uint64_t PrinterState::getOuterTemp() const {
    return outerTemp;
}

std::string PrinterState::getProfileName() const {
    return profileName;
}

int32_t PrinterState::getProfileTemp() const {
    return profileTemp;
}

float PrinterState::getFanSpeed() const {
    return fanSpeed;
}

void PrinterState::setIsOn(const bool isOn, const bool notify) {
    std::cout << "PrinterState::setIsOn(" << isOn << "), notify: " << notify << ")\n";
    this->isOn = isOn;
    if (notify) {
        notifyListeners();
    }
}

void PrinterState::setIsTempControlActive(const bool isTempControlActive, const bool notify) {
    std::cout << "PrinterState::setIsTempControlActive(" << isTempControlActive << "), notify: " << notify << ")\n";
    this->isTempControlActive = isTempControlActive;
    if (notify) {
        notifyListeners();
    }
}

void PrinterState::setInnerTopTemp(const uint64_t innerTopTemp, const bool notify) {
    std::cout << "PrinterState::setInnerTopTemp(" << innerTopTemp << "), notify: " << notify << ")\n";
    this->innerTopTemp = innerTopTemp;
    if (notify) {
        notifyListeners();
    }
}

void PrinterState::setInnerBottomTemp(const uint64_t innerBottomTemp, const bool notify) {
    std::cout << "PrinterState::setInnerBottomTemp(" << innerBottomTemp << "), notify: " << notify << ")\n";
    this->innerBottomTemp = innerBottomTemp;
    if (notify) {
        notifyListeners();
    }
}

void PrinterState::setOuterTemp(const uint64_t outerTemp, const bool notify) {
    std::cout << "PrinterState::setOuterTemp(" << outerTemp << "), notify: " << notify << ")\n";
    this->outerTemp = outerTemp;
    if (notify) {
        notifyListeners();
    }
}

void PrinterState::setProfileName(const std::string& profileName, const bool notify) {
    std::cout << "PrinterState::setProfileName(" << profileName << "), notify: " << notify << ")\n";
    this->profileName = profileName;
    if (notify) {
        notifyListeners();
    }
}

void PrinterState::setProfileTemp(const int32_t profileTemp, const bool notify) {
    std::cout << "PrinterState::setProfileTemp(" << profileTemp << "), notify: " << notify << ")\n";
    this->profileTemp = profileTemp;
    if (notify) {
        notifyListeners();
    }
}

void PrinterState::setFanSpeed(const float fanSpeed, const bool notify) {
    std::cout << "PrinterState::setFanSpeed(" << fanSpeed << "), notify: " << notify << ")\n";
    this->fanSpeed = fanSpeed;
    if (notify) {
        notifyListeners();
    }
}

std::string PrinterState::toString() const {
    std::string result = "PrinterState{";
    result += "isOn=" + std::to_string(isOn);
    result += ", isTempControlActive=" + std::to_string(isTempControlActive);
    result += ", innerTopTemp=" + std::to_string(innerTopTemp);
    result += ", innerBottomTemp=" + std::to_string(innerBottomTemp);
    result += ", outerTemp=" + std::to_string(outerTemp);
    result += ", profileName=" + profileName;
    result += ", profileTemp=" + std::to_string(profileTemp);
    result += ", fanSpeed=" + std::to_string(fanSpeed);
    result += "}";
    return result;
}