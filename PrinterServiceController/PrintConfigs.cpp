#include "PrintConfigs.h"
#include <fstream> // ifstream, ofstream
#include <filesystem>
#include <stdexcept>
#include <cstring>

namespace fs = std::filesystem;
static const char *defaultConfig("25000:PETG\n30000:PLA\n");

void PrintConfigs::evaluateLine(std::string line, std::vector<PrintConfig>& configs)
{
	if (line.empty())
		return;

	int endOfTemp = line.find_first_of(':');
	std::string temp = line.substr(0, endOfTemp);
	std::string rest = line.substr(endOfTemp + 1);
	PrintConfig config;
	config.name = rest;
	config.temperatur = std::stoi(temp);
	configs.push_back(config);
}

void PrintConfigs::loadPrintConfigs(std::string directoryPath, std::string filename, std::vector<PrintConfig>& configs)
{
	if (!fs::is_directory(directoryPath)) {
		if (!fs::create_directory(directoryPath)) {
			throw std::runtime_error("Config directory does not exist and could not be created");
			return;
		}
	}

	std::string path = directoryPath + filename;
	std::fstream file;
	file.open(path, std::fstream::in);
	if (!file) {
		// file apperantly doesnt exist yet, thus we create it
		file.open(path, std::fstream::out);
		if (!file) {
			throw std::runtime_error("Could not create the config file (" + path + ")");
			return;
		}
		else {
			file.write(defaultConfig, strlen(defaultConfig));
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
	file.close();
}

void PrintConfigs::savePrintConfigs(std::string directoryPath, std::string filename, std::vector<PrintConfig>& configs)
{
	// File and directory must exist at this point
	std::string path = directoryPath + filename;
	std::fstream file;
	file.open(path, std::fstream::trunc | std::fstream::out);

	for (PrintConfig config : configs) {
		file << config.temperatur << ':' << config.name << std::endl;
	}
	file.close();
}



