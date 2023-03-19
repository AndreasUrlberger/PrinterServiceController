#pragma once
#include "PrinterData.pb.h"
#include <thread>
#include "Utils.h"
#include "PrintConfigs.h"

#include <sys/socket.h>
#include <netinet/in.h>

#define HOST_PORT 1933

class ProtoMessageHandler
{
    enum MESSAGE_CODE
    {
        STATUS_REQUEST = 0,
        ADD_PRINT_CONFIG = 1,
        REMOVE_PRINT_CONFIG = 2,
        CHANGE_TEMP_CONTROL = 3,
        KEEP_ALIVE = 4,
        STATUS = 5
    };

    struct ClientConnection
    {
        uint64_t lastUpdate{0u};
        static constexpr size_t msgDataOffset = 2u;
        static constexpr size_t msgCodeOffset = 1u;
        uint8_t messageCode{};
        uint8_t buffer[256u];
        size_t packetLength{};
        bool connectionBroken{false};
        int socket{};
    };

private:
    int socketId;
    std::thread *connectionReceiver = nullptr;
    sockaddr_in address;
    uint64_t lastConfigChange = Utils::currentMillis();
    PrinterState state;
    std::function<void(void)> shutdownHook;
    std::function<bool(PrintConfig &)> onProfileUpdateHook;
    std::function<void(bool)> tempControlChangeHook;

    void sendCompletePacket(ClientConnection &connection);
    void statusRequest(ClientConnection &connection);
    void addPrintConfig(ClientConnection &connection);
    void removePrintConfig(ClientConnection &connection);
    void receiveCompletePacket(ClientConnection &connection);
    void getNextPacketLength(ClientConnection &connection);
    void receiveNBytes(ClientConnection &connection, size_t n, size_t offset);
    void sendStatus(ClientConnection &connection, bool sendPrintConfigs);
    void changeTempControl(ClientConnection &connection);

public:
    ProtoMessageHandler(std::function<void(void)> shutdownHook, std::function<bool(PrintConfig &)> onProfileUpdate, std::function<void(bool)> onTempControlChange);
    ~ProtoMessageHandler();

    bool start();
    void acceptConnections();
    void listenToClient(int socket);
    void updateState(PrinterState &state);
};
