#pragma once

#include <sys/socket.h>
#include <netinet/in.h> 
#include <thread>
#include <set>

#define HOST_IP "localhost"
#define HOST_PORT 1933


class PrinterServer {
private:
	int socketId;
	struct sockaddr_in address;
	int addressLength;
	std::thread *connectionReceiver = nullptr;
	std::thread* listener = nullptr;
	// The key is the file descriptor of the client socket and the value is the current deadline of the connection
	std::set<std::thread> connections;
	std::string content;

public:
	void start();
	void setContent(
		bool state,
		double progress,
		uint64_t remainingTime,
		double boardTemp,
		double nozzleTemp,
		double innerTemp,
		double outerTemp,
		std::string profileName,
		double wantedTemp
	);
	void acceptConnections();
	void listenToClient(int socket);
	PrinterServer();
};