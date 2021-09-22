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

class PrinterServer {
private:
	int socketId;
	struct sockaddr_in address;
	int addressLength;
	std::thread *connectionReceiver = nullptr;
	std::thread* listener = nullptr;
	// The key is the file descriptor of the client socket and the value is the current deadline of the connection
	std::set<std::thread> connections;
	uint64_t lastConfigChange = Utils::currentMillis();
	PrinterState state;
	std::function<void(void)> shutdownHook;
	std::function<bool(PrintConfig&)> onProfileUpdateHook;
	std::function<void(bool)> tempControlChangeHook;

	bool sendState(int socket, uint64_t& lastUpdate);
	bool applyUpdate(int socket);
	bool removeConfig(int socket);
	bool sendConfigs(int socket);
	bool sendComplete(int socket, const char* contentBuffer, int contentLength);
	std::string getContent(bool withConfig);
	bool getContentLength(int socket, int& outLength);
	bool receivePacket(int socket, char *buffer, int contentLength);
	bool tempControlChange(int socket);

public:
	void start();
	void setContent(PrinterState& state);
	void acceptConnections();
	void listenToClient(int socket);
	PrinterServer(std::function<void(void)> shutdownHook, std::function<bool(PrintConfig&)> onProfileUpdate, std::function<void(bool)> onTempControlChange);
};