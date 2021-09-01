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
	static void evaluateLine(std::string line, std::vector<PrintConfig>& configs);
public:
	static void loadPrintConfigs(std::string directoryPath, std::string filename, std::vector<PrintConfig>& configs);
	static void savePrintConfigs(std::string directoryPath, std::string filename, std::vector<PrintConfig>& configs);
};

