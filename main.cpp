#include "Logger.hpp"
#include "ServiceController.hpp"

int main(int argc, char *argv[]) {
    Logger::disableLogging();
    ServiceController controller;
    controller.run();
}
