#pragma once

#include <fstream>

class Logger
{
private:
	inline static bool logging = true;
	inline static bool open;
	inline static std::fstream file;
	static void openFile();
public:
	static void log(std::string text);
	static void close();
	static void disableLogging() {
		logging = false;
	}
	static void enableLogging() {
		logging = true;
	}
};

