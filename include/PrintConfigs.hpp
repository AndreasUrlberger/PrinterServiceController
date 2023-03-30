#pragma once
#include <string>
#include <vector>

#include "PrinterState.hpp"

struct PrintConfig {
    std::string name;
    int32_t temperature;
};

class PrintConfigs {
    static constexpr const char *const DEFAULT_CONFIG{"25000:PETG\n30000:PLA\n"};
    static constexpr const char *const CONFIG_FILE_PATH{"/usr/share/printer-service-controller/"};
    static constexpr const char *const CONFIG_FILE_NAME{"print-configs.txt"};

   private:
    inline static bool valid;
    inline static std::vector<PrintConfig> configs;
    static void evaluateLine(std::string line, std::vector<PrintConfig> &configs);
    static void loadPrintConfigs();

   public:
    static std::vector<PrintConfig> &getPrintConfigs();
    static void savePrintConfigs();
    static bool addConfig(PrintConfig &profile);
    static bool removeConfig(PrintConfig &profile, PrinterState &state);
};
