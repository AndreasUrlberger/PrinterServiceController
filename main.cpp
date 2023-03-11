#include "ServiceController.h"
#include "Logger.h"

int main(int argc, char *argv[])
{
	Logger::enableLogging();
	ServiceController controller;
	controller.run();
}
