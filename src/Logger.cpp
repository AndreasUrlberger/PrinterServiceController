#include "Logger.hpp"

#include <cstring>
#include <filesystem>
#include <stdexcept>

#include "Timing.hpp"

namespace fs = std::filesystem;
static constexpr auto LOG_FILE_PATH = "/usr/share/printer-service-controller/";
static constexpr auto LOG_FILE_NAME = "log.txt";

void Logger::openFile() {
    if (!fs::is_directory(LOG_FILE_PATH)) {
        if (!fs::create_directory(LOG_FILE_PATH)) {
            throw std::runtime_error("Log directory does not exist and could not be created");
            return;
        }
    }

    std::string path = std::string(LOG_FILE_PATH) + std::string(LOG_FILE_NAME);
    file.open(path, std::fstream::app);
    if (!file) {
        throw std::runtime_error("Could not create the log file (" + path + ")");
        return;
    }

    if (!file.good()) {
        printf("Log file is not good, cannot log anything\n");
    }
}

void Logger::log(std::string text) {
    if (!logging)
        return;

    if (!open) {
        openFile();
        open = true;
    }
    std::string message = std::to_string(Timing::currentTimeMillis());
    message += ": ";
    message += text;
    message += "\n";

    const char* cMessage = message.c_str();
    file.write(cMessage, strlen(cMessage));
    file.flush();
}

void Logger::close() {
    if (open) {
        open = false;
        file.close();
    }
}
