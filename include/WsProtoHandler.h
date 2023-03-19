#pragma once

#include "PrinterData.pb.h"
#include "PrintConfigs.h"
#include "uWebSockets/App.h"

#define HOST_PORT 1933

class WsProtoHandler
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

    struct PerSocketData
    {
        uint64_t lastUpdate{0u};
        static constexpr size_t msgDataOffset = 2u;
        static constexpr size_t msgCodeOffset = 1u;
        size_t packetLength{};
        uint8_t messageCode{};
        uint8_t buffer[256u];
        bool connectionBroken{false};
    };

private:
    uWS::App::WebSocketBehavior<PerSocketData> behaviour = uWS::App::WebSocketBehavior<PerSocketData>();
    uWS::App app = uWS::App();

    void handleIncommingMessage(std::string_view message, uWS::WebSocket<false, true, PerSocketData> *ws);
    void doStatusRequest(std::string_view message);

public:
    bool start();

    WsProtoHandler();
    ~WsProtoHandler();
};