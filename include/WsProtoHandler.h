#pragma once

#include "PrinterData.pb.h"
#include "PrintConfigs.h"
#include "uWebSockets/App.h"
#include "Utils.h"

// TODO Probably have to add thread for WS.

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
        uint64_t lastUpdate = 0u;
        static constexpr size_t msgDataOffset = 2u;
        static constexpr size_t msgCodeOffset = 1u;
        uint8_t buffer[256u];
        uWS::WebSocket<false, true, ClientConnection> *ws = nullptr;
    };

private:
    uWS::App::WebSocketBehavior<ClientConnection> behaviour = uWS::App::WebSocketBehavior<ClientConnection>();
    uWS::App app = uWS::App();
    uint64_t lastConfigChange = Utils::currentMillis();
    PrinterState state;
    std::function<void(void)> shutdownHook;
    std::function<bool(PrintConfig &)> onProfileUpdateHook;
    std::function<void(bool)> tempControlChangeHook;

    bool handleIncommingMessage(std::string_view message, uWS::WebSocket<false, true, ClientConnection> *ws);
    bool statusRequest(ClientConnection &connection, std::string_view const message);
    bool addPrintConfig(ClientConnection &connection, std::string_view const message);
    bool removePrintConfig(ClientConnection &connection, std::string_view const message);
    bool sendStatus(ClientConnection &connection, bool sendPrintConfigs);
    bool changeTempControl(ClientConnection &connection, std::string_view const message);

    void startWsServer();

public:
    bool start();
    void updateState(PrinterState &state);

    WsProtoHandler(std::function<void(void)> shutdownHook, std::function<bool(PrintConfig &)> onProfileUpdate, std::function<void(bool)> onTempControlChange);
    ~WsProtoHandler();
};