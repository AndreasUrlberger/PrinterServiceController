#include "Logger.hpp"
#include "ServiceController.hpp"

int main(int argc, char *argv[]) {
    Logger::enableLogging();
    ServiceController controller;
    controller.run();
}
