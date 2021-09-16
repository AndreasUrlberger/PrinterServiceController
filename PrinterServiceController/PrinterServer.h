#pragma once

#include <sys/socket.h>
#include <netinet/in.h> 
#include <thread>
#include "PrintConfigs.h"
#include <set>
#include "Utils.h"
#include <functional>

#define HOST_IP "localhost"
#define HOST_PORT 1933

class PServerObserver {
public:
	virtual bool onProfileUpdate(PrintConfig& profile) = 0;
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
	PServerObserver* observer = nullptr;
	uint64_t lastConfigChange = Utils::currentMillis();
	PrinterState state;
	std::function<void(void)> shutdownHook;

	bool sendState(int socket, uint64_t& lastUpdate);
	bool applyUpdate(int socket);
	bool removeConfig(int socket);
	bool sendConfigs(int socket);
	bool sendComplete(int socket, const char* contentBuffer, int contentLength);
	std::string getContent(bool withConfig);

public:
	void start();
	void setObserver(PServerObserver* observer);
	void setContent(PrinterState& state);
	void acceptConnections();
	void listenToClient(int socket);
	PrinterServer(std::function<void(void)> shutdownHook);
};