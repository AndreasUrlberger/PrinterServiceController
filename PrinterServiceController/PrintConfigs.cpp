#include "PrintConfigs.h"
#include <fstream> // ifstream, ofstream
#include <filesystem>
#include <stdexcept>
#include <cstring>
#include <algorithm>

namespace fs = std::filesystem;

static const char *defaultConfig("25000:PETG\n30000:PLA\n");
static constexpr auto CONFIG_FILE_PATH = "/usr/share/printer-service-controller/";
static constexpr auto CONFIG_FILE_NAME = "print-configs.txt";

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

void PrintConfigs::loadPrintConfigs()
{
	if (!fs::is_directory(CONFIG_FILE_PATH)) {
		if (!fs::create_directory(CONFIG_FILE_PATH)) {
			throw std::runtime_error("Config directory does not exist and could not be created");
			return;
		}
	}

	std::string path = std::string(CONFIG_FILE_PATH) + std::string(CONFIG_FILE_NAME);
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

std::vector<PrintConfig>& PrintConfigs::getPrintConfigs()
{
	if (!valid) {
		loadPrintConfigs();
		valid = true;
	}
	return configs;
}

void PrintConfigs::savePrintConfigs()
{
	// File and directory must exist at this point
	std::string path = std::string(CONFIG_FILE_PATH) + std::string(CONFIG_FILE_NAME);
	std::fstream file;
	file.open(path, std::fstream::trunc | std::fstream::out);

	for (PrintConfig config : getPrintConfigs()) {
		file << config.temperatur << ':' << config.name << std::endl;
	}
	file.close();
}

bool PrintConfigs::addConfig(PrintConfig& profile)
{
	bool hasChanged = false;
	auto profileComp = [&profile](PrintConfig& element) {return element.name.compare(profile.name); };
	std::vector<PrintConfig>& configs = getPrintConfigs();
	auto searchResult = std::find_if_not(configs.begin(), configs.end(), profileComp);
	if (searchResult == configs.end()) {
		configs.emplace(configs.begin(), profile);
		hasChanged = true;
	}
	else {
		int index = searchResult - configs.begin();
		if (index == 0) { // element already at the first place
			// only changed if the temp has changed
			hasChanged = configs.at(index).temperatur != profile.temperatur;
			configs.at(index) = profile;
		}
		else {
			hasChanged = true;
			// move config to the first place
			configs.erase(searchResult);
			configs.emplace(configs.begin(), profile);
		}
	}
	return hasChanged;
}

bool PrintConfigs::removeConfig(PrintConfig& profile)
{
	auto profileComp = [&profile](PrintConfig& element) {return element.name.compare(profile.name); };
	std::vector<PrintConfig>& configs = getPrintConfigs();
	auto searchResult = std::find_if(configs.begin(), configs.end(), profileComp);
	bool containedElement = searchResult != configs.end();
	if (containedElement) {
		configs.erase(searchResult);
	}
	return containedElement;
}



