#include "PrinterServer.h"
#include <stdexcept>
#include "Utils.h"
#include <unistd.h> 
#include <sstream>
#include <cstring>
#include <iostream>
#include <vector>
#include <string>


void staticAcceptActions(PrinterServer* server) {
	server->acceptConnections();
}

void staticListenToClient(PrinterServer* server, int socket) {
	server->listenToClient(socket);
}

void PrinterServer::setContent(
	bool state,
	double progress,
	uint64_t remainingTime,
	double boardTemp,
	double nozzleTemp,
	double innerTemp,
	double outerTemp,
	std::string profileName,
	double wantedTemp
){
	std::ostringstream ss;
	ss << "{power_state: " << (state? "true" : "false")
		<< ",progress: " << progress 
		<< ",remaining_time: " << remainingTime
		<< ",board_temp: " << boardTemp
		<< ",nozzle_temp: " << nozzleTemp
		<< ",inner_temp: " << innerTemp
		<< ",outer_temp: " << outerTemp
		<< ",temp_profile: {profile: " << profileName << ",want: " << wantedTemp << "}}";
	content = ss.str();
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
	char* readBuffer[1];
	while (read(socket, readBuffer, 1) > 0) {
		const char *cContent = content.c_str();
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
		if (sendProgress != sendLength) {
			// connection closed while sending
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
