#pragma once
#include <string>
#include <vector>

struct PrintConfig {
	std::string name;
	int32_t temperatur;
};

class PrintConfigs
{
private:
	inline static bool valid;
	inline static std::vector<PrintConfig> configs;
	static void evaluateLine(std::string line, std::vector<PrintConfig>& configs);
public:
	static void loadPrintConfigs();
	static std::vector<PrintConfig>& getPrintConfigs();
	static void savePrintConfigs();
};

