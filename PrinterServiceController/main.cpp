#include "ServiceController.h"
#include "Logger.h"

int main()
{
	Logger::disableLogging();
	ServiceController controller;
	controller.run();
}
