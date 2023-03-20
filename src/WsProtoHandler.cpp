#include "WsProtoHandler.h"

WsProtoHandler::WsProtoHandler(std::function<void(void)> shutdownHook, std::function<bool(PrintConfig &)> onProfileUpdate, std::function<void(bool)> onTempControlChange)
{
    this->shutdownHook = shutdownHook;
    onProfileUpdateHook = onProfileUpdate;
    tempControlChangeHook = onTempControlChange;
}

WsProtoHandler::~WsProtoHandler()
{
}

bool WsProtoHandler::start()
{
    startWsServer();
    return true;
}

void WsProtoHandler::startWsServer()
{
    std::cout << "Called startWsServer\n";
    behaviour.compression = uWS::DISABLED,
    behaviour.maxPayloadLength = 16 * 1024,
    behaviour.idleTimeout = 120,
    behaviour.closeOnBackpressureLimit = false,
    behaviour.resetIdleTimeoutOnSend = true,
    behaviour.upgrade = [](uWS::HttpResponse<false> *res, uWS::HttpRequest *req, us_socket_context_t *context)
    {
        std::cout << "UPGRADE\n";

        ClientConnection data;
        res->upgrade<ClientConnection>(std::move(data),
                                       req->getHeader("sec-websocket-key"),
                                       req->getHeader("sec-websocket-protocol"),
                                       req->getHeader("sec-websocket-extensions"),
                                       context);
    };
    behaviour.open = [](auto *ws)
    {
        std::cout << "OPEN\n";
    };
    behaviour.message = [this](uWS::WebSocket<false, true, ClientConnection> *ws, std::string_view message, uWS::OpCode opCode)
    {
        std::cout << "MESSAGE\n";
        this->handleIncommingMessage(message, ws);
        // ws->send(message, opCode);
    };
    behaviour.close = [](auto *ws, int code, std::string_view message)
    {
        std::cout << "CLOSE\n";
    };
    behaviour.subscription = [](uWS::WebSocket<false, true, WsProtoHandler::ClientConnection> *, std::string_view, int, int)
    {
        std::cout << "SUBSCRIPTION\n";
    };

    app.addServerName("Printer-3D");
    app.listen(HOST_PORT, [](auto *listen_socket)
               {
        if (listen_socket) {
            std::cout << "Listening on port " << HOST_PORT << std::endl;

        }else {
            std::cout << "Thread " << std::this_thread::get_id() << " failed to listen on port "<< HOST_PORT << std::endl;
        } });

    app.ws<ClientConnection>("/*", std::move(behaviour));

    std::cout << "Next step is running the app\n";

    app.run();

    std::cout << "ConstructorFailed: " << (app.constructorFailed() ? "yes" : "no") << "\n";
    std::cout << "Ran the app\n";
}

bool WsProtoHandler::handleIncommingMessage(std::string_view message, uWS::WebSocket<false, true, ClientConnection> *ws)
{
    std::cout << "StatusRequest: msgLength: " + std::to_string(message.length()) << "\n";

    constexpr size_t msgDataOffset = 2u;
    ClientConnection &connection = *ws->getUserData();
    connection.ws = ws;
    const uint8_t messageCode = message[connection.msgCodeOffset];
    const std::string_view dataString = message.substr(msgDataOffset);

    bool connectionAlive = false;

    switch (messageCode)
    {
    case static_cast<uint8_t>(MESSAGE_CODE::STATUS_REQUEST):
        connectionAlive = statusRequest(connection, dataString);
        break;

    case static_cast<uint8_t>(MESSAGE_CODE::ADD_PRINT_CONFIG):
        connectionAlive = addPrintConfig(connection, dataString);
        break;

    case static_cast<uint8_t>(MESSAGE_CODE::REMOVE_PRINT_CONFIG):
        connectionAlive = removePrintConfig(connection, dataString);
        break;

    case static_cast<uint8_t>(MESSAGE_CODE::CHANGE_TEMP_CONTROL):
        connectionAlive = changeTempControl(connection, dataString);
        break;

    case static_cast<uint8_t>(MESSAGE_CODE::KEEP_ALIVE):
        // Nothing to do.
        break;

    default:
        std::cerr << "ERROR: Invalid message code: " << static_cast<uint32_t>(messageCode) << "\n";
        break;
    }

    return connectionAlive;
}

bool WsProtoHandler::sendStatus(ClientConnection &connection, bool sendPrintConfigs)
{
    Printer::PrinterStatus printerStatus;
    printerStatus.set_is_temp_control_active(state.tempControl);
    printerStatus.set_temperature_outside(state.outerTemp);
    printerStatus.set_temperature_inside_top(state.innerTemp);
    printerStatus.set_temperature_inside_bottom(state.innerTemp);
    Printer::PrintConfig *currentPrintConfig = new Printer::PrintConfig();
    currentPrintConfig->set_name(state.profileName);
    currentPrintConfig->set_temperature(state.profileTemp);
    printerStatus.set_allocated_current_print_config(currentPrintConfig);

    if (sendPrintConfigs || (connection.lastUpdate <= lastConfigChange))
    {
        lastConfigChange = Utils::currentMillis();
        std::vector<PrintConfig> configs = PrintConfigs::getPrintConfigs();
        for (auto config = configs.begin(); config != configs.end(); config++)
        {
            Printer::PrintConfig *printConfig = printerStatus.add_print_configs();
            printConfig->set_name(config->name);
            printConfig->set_temperature(config->temperature);
        }
    }

    // Send protobuf (use the same buffer again).
    const size_t dataSize = printerStatus.ByteSizeLong();
    const size_t packetLength = dataSize + connection.msgCodeOffset;
    if ((packetLength + connection.msgDataOffset) > 256u)
    {
        std::cerr << "FATAL ERROR: The message size is larger than 256 bytes. The size field is only one byte long, it cannot hold more data.\n";
        return false;
    }

    connection.buffer[0u] = static_cast<uint8_t>(packetLength);
    connection.buffer[connection.msgCodeOffset] = static_cast<uint8_t>(MESSAGE_CODE::STATUS);
    if (!printerStatus.SerializeToArray(connection.buffer + connection.msgDataOffset, dataSize))
    {
        std::cerr << "ERROR: Failed to serialize StatusRequest message to buffer array\n";
        return false;
    }

    size_t fullLength = dataSize + connection.msgDataOffset;
    const char *const message = reinterpret_cast<char *>(&(connection.buffer));
    std::string_view outMessage = std::string_view(message, fullLength);
    connection.ws->send(outMessage);

    return true;
}

bool WsProtoHandler::statusRequest(ClientConnection &connection, std::string_view const message)
{
    std::cout << "Data bytes received, length: " << message.length() << " : [";
    for (size_t index = 0u; index < message.length(); index = index + 1u)
    {
        std::cout << static_cast<uint32_t>(message[index]) << ", ";
    }
    std::cout << "]\n";

    Printer::StatusRequest statusRequest;
    if (!statusRequest.ParseFromArray(message.begin(), message.length()))
    {
        std::cerr << "ERROR: Failed to parse StatusRequest message from input array\n";
        return false;
    }

    const bool sendPrintConfigs = statusRequest.include_print_configs();

    std::cout << "SendPrintConfigs: " << (sendPrintConfigs ? "yes" : "no") << "\n";

    if (sendStatus(connection, sendPrintConfigs))
    {
        return true;
    }
    else
    {
        std::cerr << "ERROR: Failed to send status message.\n";
        return false;
    }
}

bool WsProtoHandler::addPrintConfig(ClientConnection &connection, std::string_view const message)
{
    PrintConfig config;

    // Read in message with protobuff.
    Printer::AddPrintConfig addPrintConfig;
    if (!addPrintConfig.ParseFromArray(message.begin(), message.length()))
    {
        std::cerr << "ERROR: Failed to parse AddPrintConfig message from input array\n";
        return false;
    }

    Printer::PrintConfig receivedConfig = addPrintConfig.print_config();
    config.name = receivedConfig.name();
    config.temperature = receivedConfig.temperature();

    // TODO inspect
    bool hasChanged = onProfileUpdateHook(config);
    if (hasChanged)
    {
        // Not thread safe --> There could be mistakes.
        lastConfigChange = Utils::currentMillis();
    }

    if (sendStatus(connection, hasChanged))
    {
        return true;
    }
    else
    {
        std::cerr << "ERROR: Failed to send status message.\n";
        return false;
    }
}

bool WsProtoHandler::removePrintConfig(ClientConnection &connection, std::string_view const message)
{
    PrintConfig config;

    // Read in message with protobuff.
    Printer::RemovePrintConfig removePrintConfig;
    if (!removePrintConfig.ParseFromArray(message.begin(), message.length()))
    {
        std::cerr << "ERROR: Failed to parse RemovePrintConfig message from input array\n";
        return false;
    }

    Printer::PrintConfig receivedConfig = removePrintConfig.print_config();
    config.name = receivedConfig.name();
    config.temperature = receivedConfig.temperature();

    bool hasChanged = PrintConfigs::removeConfig(config);
    if (hasChanged)
    {
        lastConfigChange = Utils::currentMillis();
    }

    if (sendStatus(connection, hasChanged))
    {
        return true;
    }
    else
    {
        std::cerr << "ERROR: Failed to send status message.\n";
        return false;
    }
}

bool WsProtoHandler::changeTempControl(ClientConnection &connection, std::string_view const message)
{

    // Read in message with protobuff.
    Printer::ChangeTempControl changeTempControl;
    if (!changeTempControl.ParseFromArray(message.begin(), message.length()))
    {
        std::cerr << "ERROR: Failed to parse ChangeTempControl message from input array\n";
        return false;
    }

    // Change temp control
    tempControlChangeHook(changeTempControl.isactive());

    if (sendStatus(connection, false))
    {
        return true;
    }
    else
    {
        std::cerr << "ERROR: Failed to send status message.\n";
        return false;
    }
}

void WsProtoHandler::updateState(PrinterState &state)
{
    this->state = state;
}