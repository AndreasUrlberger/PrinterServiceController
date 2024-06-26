#include "PrintConfigs.hpp"

#include <assert.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>  // ifstream, ofstream
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;

void PrintConfigs::evaluateLine(const std::string line, std::vector<PrintConfig> &configs) {
    if (line.empty())
        return;

    const size_t endOfTemp = line.find_first_of(':');
    const std::string temp = line.substr(0, endOfTemp);
    const std::string rest = line.substr(endOfTemp + 1);
    PrintConfig config;
    config.name = rest;
    config.temperature = std::stoi(temp);
    configs.push_back(config);
}

void PrintConfigs::loadPrintConfigs() {
    if (!fs::is_directory(CONFIG_FILE_PATH)) {
        if (!fs::create_directory(CONFIG_FILE_PATH)) {
            throw std::runtime_error("Config directory does not exist and could not be created");
            return;
        }
    }

    const std::string path = std::string(CONFIG_FILE_PATH) + std::string(CONFIG_FILE_NAME);
    std::fstream file;
    file.open(path, std::fstream::in);
    if (!file) {
        // file apperantly doesnt exist yet, thus we create it
        file.open(path, std::fstream::out);
        if (!file) {
            throw std::runtime_error("Could not create the config file (" + path + ")");
            return;
        } else {
            file.write(DEFAULT_CONFIG, strlen(DEFAULT_CONFIG));
            file.close();
        }
        file.open(path, std::fstream::in);
    }

    if (!file.good()) {
        printf("Config file is not good, cannot load print configs\n");
    }

    std::string line;
    while (getline(file, line)) {
        evaluateLine(line, configs);
    }
    if (configs.size() == 0) {
        PrintConfig config;
        config.name = "PETG";
        config.temperature = INT32_C(25'000);
        configs.emplace(configs.begin(), config);
    }
    file.close();
}

std::vector<PrintConfig> &PrintConfigs::getPrintConfigs() {
    if (!valid) {
        loadPrintConfigs();
        valid = true;
    }
    return configs;
}

void PrintConfigs::savePrintConfigs() {
    assert(valid);  // PrintConfigs is not valid, file or directory for saves does not exist.

    const std::string path = std::string(CONFIG_FILE_PATH) + std::string(CONFIG_FILE_NAME);
    std::fstream file;
    file.open(path, std::fstream::out);

    std::ostringstream ss;
    for (const PrintConfig config : getPrintConfigs()) {
        ss << config.temperature << ':' << config.name << std::endl;
    }
    std::string outputString = ss.str();
    const char *const configsText = outputString.c_str();
    file.write(configsText, strlen(configsText));
    file.close();
}

// Probably not thread safe.
bool PrintConfigs::addConfig(PrintConfig &profile) {
    bool hasChanged = false;
    auto profileComp = [&profile](PrintConfig &element) { return element.name.compare(profile.name); };
    std::vector<PrintConfig> &configs = getPrintConfigs();
    auto searchResult = std::find_if_not(configs.begin(), configs.end(), profileComp);
    if (searchResult == configs.end()) {
        configs.emplace(configs.begin(), profile);
        hasChanged = true;
    } else {
        int index = searchResult - configs.begin();
        if (index == 0) {  // element already at the first place
            // only changed if the temp has changed
            hasChanged = configs.at(index).temperature != profile.temperature;
            configs.at(index) = profile;
        } else {
            hasChanged = true;
            // move config to the first place
            configs.erase(searchResult);
            configs.emplace(configs.begin(), profile);
        }
    }
    if (hasChanged) {
        savePrintConfigs();
    }
    return hasChanged;
}

bool PrintConfigs::removeConfig(PrintConfig &profile, PrinterState &state) {
    std::vector<PrintConfig> &configs = getPrintConfigs();
    if (configs.size() <= 1)
        return false;

    const auto profileComp = [&profile](PrintConfig &element) { return element.name.compare(profile.name); };
    const auto searchResult = std::find_if_not(configs.begin(), configs.end(), profileComp);
    const bool containsElement = searchResult != configs.end();
    if (containsElement) {
        // Make sure the current profile is not a removed profile.
        if (state.getProfileName() == profile.name && state.getProfileTemp() == profile.temperature) {
            // Find any other profile and set it as the current profile.
            const auto otherElement = std::find_if(configs.begin(), configs.end(), profileComp);
            state.setProfileName(otherElement->name, false);
            state.setProfileTemp(otherElement->temperature);
        }
        configs.erase(searchResult);
        savePrintConfigs();
    }
    return containsElement;
}
