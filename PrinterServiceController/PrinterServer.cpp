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
constexpr char REQUEST_CONFIGS_CODE = 2;


void staticAcceptActions(PrinterServer* server) {
	server->acceptConnections();
}

void staticListenToClient(PrinterServer* server, int socket) {
	server->listenToClient(socket);
}

void PrinterServer::setContent(PrinterState& state){
	std::ostringstream ss;
	ss << "{power_state: " << (state.state? "true" : "false")
		<< ",progress: " << state.progress 
		<< ",remaining_time: " << state.remainingTime
		<< ",board_temp: " << state.boardTemp
		<< ",nozzle_temp: " << state.nozzleTemp
		<< ",inner_temp: " << state.innerTemp
		<< ",outer_temp: " << state.outerTemp
		<< ",temp_profile: {profile: " << state.profileName << ",want: " << state.profileTemp << "}}";
	content = ss.str();
}

bool PrinterServer::sendState(int socket)
{
	const char* cContent = content.c_str();
	int contentLength = content.length();
	int sendLength = contentLength + 2;

	char message[sendLength];
	message[0] = static_cast<char>(contentLength >> 8); // hi
	message[1] = static_cast<char>(contentLength & 0xFF); // lo

	for (int index = 0; index < contentLength; ++index) {
		char c = *(cContent + index);
		message[index + 2] = c;
	}

	int bytesSent = 1;
	int sendProgress = 0;
	while (bytesSent > 0 && sendProgress < sendLength) {
		bytesSent = write(socket, message, sendLength - sendProgress);
		sendProgress += bytesSent;
	}
	
	// if false, connection has closed
	return sendProgress == sendLength;
}

bool PrinterServer::applyUpdate(int socket)
{
	// Get content length
	int bytesReceived = 1;
	int progress = 0;
	char lengthBuffer[2];
	while (bytesReceived > 0 && progress < 2) {
		bytesReceived = read(socket, lengthBuffer, 2 - progress);
		progress += bytesReceived;
	}
	if (progress != 2) { // connection has failed
		return false;
	}
	int contentLength = static_cast<int>(lengthBuffer[0]) << 8; // high
	contentLength += static_cast<int>(lengthBuffer[1]); // + low

	// Get profile
	bytesReceived = 1;
	progress = 0;
	char buffer[contentLength];
	while (bytesReceived > 0 && progress < contentLength) {
		bytesReceived = read(socket, buffer, contentLength - progress);
		progress += bytesReceived;
	}
	if (progress != contentLength) { // connection has failed
		return false;
	}

	std::string input{ buffer, static_cast<size_t>(contentLength) };
	int endOfTemp = input.find_first_of(':');
	int temp = std::stoi(input.substr(0, endOfTemp));
	std::string name = input.substr(endOfTemp + 1);
	printf("namelength: %d", name.length());
	if (observer != nullptr) {
		PrintConfig config;
		config.name = name;
		config.temperatur = temp;
		observer->onProfileUpdate(config);
	}
	return true;
}

bool PrinterServer::sendConfigs(int socket) {
	std::vector<PrintConfig> configs = PrintConfigs::getPrintConfigs();
	std::ostringstream ss;
	for (auto config : configs) {
		ss << config.name << ":" << config.temperatur << "\n";
	}

	std::string contentStr = ss.str();
	const char *configsBuffer = contentStr.c_str();
	int contentLength = strlen(configsBuffer);	
	
	return sendComplete(socket, configsBuffer, contentLength);
}

bool PrinterServer::sendComplete(int socket, const char* contentBuffer, int contentLength) {
	int sendLength = contentLength + 2;
	char* buffer = new char[sendLength];
	memcpy(buffer + 2 * sizeof(char), contentBuffer, contentLength);
	buffer[0] = static_cast<char>(contentLength >> 8); // hi
	buffer[1] = static_cast<char>(contentLength & 0xFF); // lo
	int bytesSent = 1;
	int sendProgress = 0;
	while (bytesSent > 0 && sendProgress < sendLength) {
		bytesSent = write(socket, buffer, sendLength - sendProgress);
		sendProgress += bytesSent;
	}
	free(buffer);
	return sendProgress == sendLength;
}

void PrinterServer::start()
{
	if (connectionReceiver == nullptr) {
		socketId = socket(AF_INET, SOCK_STREAM, 0);
		if (socketId == 0) {
			throw std::runtime_error("Could not create socket");
			return;
		}
		int opt = 1;
		if (setsockopt(socketId, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
		{
			perror("setsockopt");
			exit(EXIT_FAILURE);
		}
		if (bind(socketId, (struct sockaddr*)&address, sizeof(address)) < 0)
		{
			throw std::runtime_error("bind failed");
			return;
		}
		if (listen(socketId, 3) < 0)
		{
			throw std::runtime_error("listening failed");
			return;
		}
		connectionReceiver = new std::thread(staticAcceptActions, this);
	}
}

void PrinterServer::setObserver(PServerObserver* observer)
{
	this->observer = observer;
}

void PrinterServer::acceptConnections()
{
	int new_socket;
	while ((new_socket = accept(socketId, (struct sockaddr*)&address, (socklen_t*)&addressLength)) >= 0)
	{
		std::cout << "accepting new connection" << std::endl;
		std::thread newListener = std::thread(staticListenToClient, this, new_socket);
		newListener.detach();
	}
	throw std::runtime_error("accepting a connection failed");
}

void PrinterServer::listenToClient(int socket)
{
	char readBuffer[1];
	bool connectionAlive = true;
	while (read(socket, readBuffer, 1) > 0 && connectionAlive) {
		switch (readBuffer[0]) {
		case REQUEST_CODE: connectionAlive = sendState(socket);
			break;
		case UPDATE_CODE: connectionAlive = applyUpdate(socket);
			connectionAlive &= sendState(socket);
			break;
		case REQUEST_CONFIGS_CODE: connectionAlive = sendConfigs(socket);
			break;
		default: 
			break;
		}
	}
	std::cout << "connection ended" << std::endl;
}

PrinterServer::PrinterServer()
{
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(HOST_PORT);
	addressLength = sizeof(address);
}
