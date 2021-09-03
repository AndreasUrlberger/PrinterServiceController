#pragma once

#include <sys/socket.h>
#include <netinet/in.h> 
#include <thread>
#include "PrintConfigs.h"
#include <set>
#include "Utils.h"

#define HOST_IP "localhost"
#define HOST_PORT 1933

class PServerObserver {
public:
	virtual void onProfileUpdate(PrintConfig& profile) = 0;
};

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
	PServerObserver* observer = nullptr;

	bool sendState(int socket);
	bool applyUpdate(int socket);
	bool sendConfigs(int socket);
	bool sendComplete(int socket, const char* contentBuffer, int contentLength);

public:
	void start();
	void setObserver(PServerObserver* observer);
	void setContent(PrinterState& state);
	void acceptConnections();
	void listenToClient(int socket);
	PrinterServer();
};