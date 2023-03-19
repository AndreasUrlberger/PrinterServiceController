#pragma once
#include <string>
#include <vector>

struct PrintConfig
{
	std::string name;
	int32_t temperature;
};

class PrintConfigs
{
private:
	inline static bool valid;
	inline static std::vector<PrintConfig> configs;
	static void evaluateLine(std::string line, std::vector<PrintConfig> &configs);
	static void loadPrintConfigs();

public:
	static std::vector<PrintConfig> &getPrintConfigs();
	static void savePrintConfigs();
	static bool addConfig(PrintConfig &profile);
	static bool removeConfig(PrintConfig &profile);
};
