#include "WsProtoHandler.h"

bool WsProtoHandler::start()
{
    behaviour.compression = uWS::SHARED_COMPRESSOR,
    behaviour.maxPayloadLength = 16 * 1024,
    behaviour.idleTimeout = 10,
    behaviour.closeOnBackpressureLimit = false,
    behaviour.resetIdleTimeoutOnSend = true,
    behaviour.upgrade = [](uWS::HttpResponse<false> *res, uWS::HttpRequest *req, us_socket_context_t *context)
    {
        std::cout << "UPGRADE\n";

        PerSocketData data;
        res->upgrade<PerSocketData>(std::move(data),
                                    req->getHeader("sec-websocket-key"),
                                    req->getHeader("sec-websocket-protocol"),
                                    req->getHeader("sec-websocket-extensions"),
                                    context);
    };
    behaviour.open = [](auto *ws)
    {
        std::cout << "OPEN\n";
    };
    behaviour.message = [this](uWS::WebSocket<false, true, PerSocketData> *ws, std::string_view message, uWS::OpCode opCode)
    {
        std::cout << "MESSAGE\n";
        this->handleIncommingMessage(message, ws);
        ws->send(message, opCode);
    };
    behaviour.close = [](auto *ws, int code, std::string_view message)
    {
        std::cout << "CLOSE\n";
    };

    app.addServerName("Printer-3D");
    app.listen(HOST_PORT, [](auto *listen_socket)
               {
        if (listen_socket) {
            std::cout << "Listening on port " << HOST_PORT << std::endl;

        } });

    app.ws<PerSocketData>("/*", std::move(behaviour));
    app.run();

    return true;
}

void WsProtoHandler::handleIncommingMessage(std::string_view message, uWS::WebSocket<false, true, PerSocketData> *ws)
{
    std::cout << "MESSAGE\n";
    const PerSocketData *socketData = ws->getUserData();
    const uint8_t messageCode = message[socketData->msgCodeOffset];

    switch (messageCode)
    {
    case static_cast<uint8_t>(MESSAGE_CODE::STATUS_REQUEST):
        doStatusRequest(message);
        break;

    case static_cast<uint8_t>(MESSAGE_CODE::ADD_PRINT_CONFIG):
        std::cout << "Add print config\n";
        break;

    case static_cast<uint8_t>(MESSAGE_CODE::REMOVE_PRINT_CONFIG):
        // removePrintConfig(connection);
        break;

    case static_cast<uint8_t>(MESSAGE_CODE::CHANGE_TEMP_CONTROL):
        // changeTempControl(connection);
        break;

    case static_cast<uint8_t>(MESSAGE_CODE::KEEP_ALIVE):
        // Do nothing, keep alive is done automatically.
        break;

    default:
        std::cerr << "ERROR: Invalid message code: " << static_cast<uint32_t>(messageCode) << "\n";
        break;
    }
}

void WsProtoHandler::doStatusRequest(std::string_view message)
{
    size_t msgDataOffset = 2u;

    std::cout << "Status Request with length: " << message.length() << "\n";
    auto dataString = message.substr(msgDataOffset);
    Printer::StatusRequest statusRequest;
    statusRequest.ParseFromString(dataString.data());
    if (!statusRequest.ParseFromString(dataString.data()))
    {
        std::cerr << "ERROR: Failed to parse StatusRequest message from input array\n";
        return;
    }
}

WsProtoHandler::WsProtoHandler()
{
}

WsProtoHandler::~WsProtoHandler()
{
}
