#include "PrinterServer.h"
#include <stdexcept>
#include <unistd.h>
#include <sstream>
#include <cstring>
#include <iostream>
#include <vector>
#include <string>

constexpr char REQUEST_CODE = 0;
constexpr char UPDATE_CODE = 1;
constexpr char REMOVE_CONFIG_CODE = 2;
constexpr char SHUTDOWN_CODE = 3;
constexpr char TEMP_CONTROL_CODE = 4;

void PrinterServer::setContent(PrinterState &printerState)
{
	state = printerState;
}

std::string PrinterServer::getContent(bool withConfigs)
{
	std::ostringstream ss;
	ss << "{power_state: " << (state.state ? "true" : "false")
	   << ",tempControl: " << (state.tempControl ? "true" : "false")
	   << ",progress: " << state.progress
	   << ",remaining_time: " << state.remainingTime
	   << ",board_temp: " << state.boardTemp
	   << ",nozzle_temp: " << state.nozzleTemp
	   << ",inner_temp: " << state.innerTemp
	   << ",outer_temp: " << state.outerTemp
	   << ",temp_profile: {profile: " << state.profileName << ",want: " << state.profileTemp << "}";
	if (withConfigs)
	{
		std::vector<PrintConfig> configs = PrintConfigs::getPrintConfigs();
		ss << ",configs: [";
		for (auto config = configs.begin(); config != configs.end(); config++)
		{
			if (config != configs.begin())
				ss << ",";
			ss << "[" << config.base()->name << ", " << config.base()->temperature << "]";
		}
		ss << "]";
	}
	ss << "}";
	return ss.str();
}

bool PrinterServer::sendState(int socket, uint64_t &lastUpdate)
{
	bool needConfigs = lastUpdate < lastConfigChange;
	if (needConfigs)
	{
		lastUpdate = Utils::currentMillis();
	}
	std::string content = getContent(needConfigs);
	const char *cContent = content.c_str();
	int contentLength = content.length();

	return sendComplete(socket, cContent, contentLength);
}

bool PrinterServer::getContentLength(int socket, int &outLength)
{
	int bytesReceived = 1;
	int progress = 0;
	char lengthBuffer[2];
	while (bytesReceived > 0 && progress < 2)
	{
		bytesReceived = read(socket, lengthBuffer, 2 - progress);
		progress += bytesReceived;
	}
	if (progress != 2)
	{ // connection has failed
		return false;
	}
	int contentLength = static_cast<int>(lengthBuffer[0]) << 8; // high
	contentLength += static_cast<int>(lengthBuffer[1]);			// + low
	outLength = contentLength;
	return true;
}

bool PrinterServer::receivePacket(int socket, char *buffer, int contentLength)
{
	int bytesReceived = 1;
	int progress = 0;
	while (bytesReceived > 0 && progress < contentLength)
	{
		bytesReceived = read(socket, buffer, contentLength - progress);
		progress += bytesReceived;
	}
	return progress == contentLength;
}

bool PrinterServer::applyUpdate(int socket)
{
	// get Content
	int contentLength;
	if (!getContentLength(socket, contentLength))
	{
		return false;
	}
	if (contentLength == 0)
	{
		return true;
	}
	char buffer[contentLength];
	bool succ = receivePacket(socket, &buffer[0], contentLength);
	if (!succ)
	{
		return false;
	}

	std::string input{buffer, static_cast<size_t>(contentLength)};
	int endOfTemp = input.find_first_of(':');
	printf("applyUpdate: endOfTemp: %d, input: '%s'\n", endOfTemp, input.c_str());
	int temp = std::stoi(input.substr(0, endOfTemp));
	printf("applyUpdate: endOfTemp + 1: %d, input: '%s'\n", (endOfTemp + 1), input.c_str());
	std::string name = input.substr(endOfTemp + 1);
	PrintConfig config;
	config.name = name;
	config.temperature = temp;
	bool hasChanged = onProfileUpdateHook(config);
	if (hasChanged)
	{
		lastConfigChange = Utils::currentMillis();
	}
	return true;
}

bool PrinterServer::removeConfig(int socket)
{
	// get Content
	int contentLength;
	if (!getContentLength(socket, contentLength))
	{
		return false;
	}
	if (contentLength == 0)
	{
		return true;
	}
	char buffer[contentLength];
	if (!receivePacket(socket, &buffer[0], contentLength))
	{
		return false;
	}

	std::string name{buffer, static_cast<size_t>(contentLength)};
	PrintConfig config;
	config.name = name;
	config.temperature = 0;
	bool hasChanged = PrintConfigs::removeConfig(config);
	if (hasChanged)
	{
		lastConfigChange = Utils::currentMillis();
	}
	return true;
}

bool PrinterServer::sendConfigs(int socket)
{
	std::vector<PrintConfig> configs = PrintConfigs::getPrintConfigs();
	std::ostringstream ss;
	for (auto config : configs)
	{
		ss << config.name << ":" << config.temperature << "\n";
	}

	std::string contentStr = ss.str();
	const char *configsBuffer = contentStr.c_str();
	int contentLength = strlen(configsBuffer);

	return sendComplete(socket, configsBuffer, contentLength);
}

bool PrinterServer::sendComplete(int socket, const char *contentBuffer, int contentLength)
{
	int sendLength = contentLength + 2;
	char *buffer = new char[sendLength];
	memcpy(buffer + 2 * sizeof(char), contentBuffer, contentLength);
	buffer[0] = static_cast<char>(contentLength >> 8);	 // hi
	buffer[1] = static_cast<char>(contentLength & 0xFF); // lo
	int bytesSent = 1;
	int sendProgress = 0;
	while (bytesSent > 0 && sendProgress < sendLength)
	{
		bytesSent = write(socket, buffer, sendLength - sendProgress);
		sendProgress += bytesSent;
	}
	free(buffer);
	return sendProgress == sendLength;
}

void PrinterServer::start()
{
	if (connectionReceiver == nullptr)
	{
		socketId = socket(AF_INET, SOCK_STREAM, 0);
		if (socketId == 0)
		{
			throw std::runtime_error("Could not create socket");
			return;
		}
		int opt = 1;
		if (setsockopt(socketId, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
		{
			perror("setsockopt");
			exit(EXIT_FAILURE);
		}
		if (bind(socketId, (struct sockaddr *)&address, sizeof(address)) < 0)
		{
			throw std::runtime_error("bind failed");
			return;
		}
		if (listen(socketId, 3) < 0)
		{
			throw std::runtime_error("listening failed");
			return;
		}
		connectionReceiver = new std::thread(Utils::callLambda, [this]()
											 { acceptConnections(); });
	}
}

void PrinterServer::acceptConnections()
{
	int new_socket;
	while ((new_socket = accept(socketId, (struct sockaddr *)&address, (socklen_t *)&addressLength)) >= 0)
	{
		std::thread newListener = std::thread(Utils::callLambda, [this, new_socket]()
											  { listenToClient(new_socket); });
		newListener.detach();
	}
	throw std::runtime_error("accepting a connection failed");
}

bool PrinterServer::tempControlChange(int socket)
{
	char readBuffer[1];
	if (read(socket, readBuffer, 1) > 0)
	{
		tempControlChangeHook(readBuffer[0]);
		return true;
	}
	else
	{
		return false;
	}
}

void PrinterServer::listenToClient(int socket)
{
	char readBuffer[1];
	bool connectionAlive = true;
	uint64_t lastUpdate = 0;
	while (read(socket, readBuffer, 1) > 0 && connectionAlive)
	{
		switch (readBuffer[0])
		{
		case REQUEST_CODE:
			connectionAlive = sendState(socket, lastUpdate);
			break;
		case UPDATE_CODE:
			connectionAlive = applyUpdate(socket);
			connectionAlive &= sendState(socket, lastUpdate);
			break;
		case REMOVE_CONFIG_CODE:
			connectionAlive = removeConfig(socket);
			connectionAlive &= sendState(socket, lastUpdate);
			break;
		case SHUTDOWN_CODE:
			connectionAlive = false;
			shutdownHook();
			break;
		case TEMP_CONTROL_CODE:
			connectionAlive = tempControlChange(socket);
			connectionAlive &= sendState(socket, lastUpdate);
			break;
		default:
			break;
		}
	}
}

PrinterServer::PrinterServer(std::function<void(void)> shutdownHook, std::function<bool(PrintConfig &)> onProfileUpdate, std::function<void(bool)> onTempControlChange)
{
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(HOST_PORT);
	addressLength = sizeof(address);

	this->shutdownHook = shutdownHook;
	onProfileUpdateHook = onProfileUpdate;
	tempControlChangeHook = onTempControlChange;
}
