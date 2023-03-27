#include "ProtoMessageHandler.h"

#include <unistd.h>

#include "Logger.h"
#include "string"

ProtoMessageHandler::ProtoMessageHandler(std::function<void(void)> shutdownHook, std::function<bool(PrintConfig &)> onProfileUpdate, std::function<void(bool)> onTempControlChange) {
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(HOST_PORT);

    this->shutdownHook = shutdownHook;
    onProfileUpdateHook = onProfileUpdate;
    tempControlChangeHook = onTempControlChange;
}

ProtoMessageHandler::~ProtoMessageHandler() {
}

bool ProtoMessageHandler::start() {
    constexpr int opt = 1;

    if (connectionReceiver == nullptr) {
        socketId = socket(AF_INET, SOCK_STREAM, 0);
        if (socketId <= 0) {
            socketId = 0;
            std::cerr << "Could not create socket\n";
            return false;
        }
        if (setsockopt(socketId, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
            std::cerr << "ERROR: setsockopt failed: " << strerror(errno) << "\n";
            close(socketId);
            socketId = 0;
            return false;
        }
        if (bind(socketId, reinterpret_cast<sockaddr *>(&address), sizeof(address)) < 0) {
            std::cerr << "ERROR: bind failed\n";
            close(socketId);
            socketId = 0;
            return false;
        }
        if (listen(socketId, 3) < 0) {
            close(socketId);
            socketId = 0;
            std::cerr << "ERROR: listening failed/n";
            return false;
        }
        connectionReceiver = new std::thread(Utils::callLambda, [this]() { acceptConnections(); });

        return true;
    } else {
        return true;
    }
}

void ProtoMessageHandler::acceptConnections() {
    int clientSocket;
    sockaddr *addressPtr = reinterpret_cast<sockaddr *>(&address);
    socklen_t addressLength = sizeof(address);

    for (clientSocket = accept(socketId, addressPtr, &addressLength); clientSocket >= 0; clientSocket = accept(socketId, addressPtr, &addressLength)) {
        std::thread newListener = std::thread(Utils::callLambda, [this, clientSocket]() { listenToClient(clientSocket); });
        newListener.detach();
    }

    std::cerr << "ERROR: Accepting a connection failed\n";
    exit(EXIT_FAILURE);
}

void ProtoMessageHandler::listenToClient(int socket) {
    ClientConnection connection;
    connection.socket = socket;

    while (!connection.connectionBroken) {
        // if (receiveCompletePacket(socket, buffer + pktDataOffset, packetLength))
        receiveCompletePacket(connection);
        if (connection.connectionBroken) {
            std::cerr << "ERROR: Failed to receive complete packet.\n";
            break;
        }

        std::cout << "Received complete package with messageCode: " + std::to_string(connection.messageCode) << "\n";

        switch (connection.messageCode) {
            case static_cast<uint8_t>(MESSAGE_CODE::STATUS_REQUEST):
                statusRequest(connection);
                break;

            case static_cast<uint8_t>(MESSAGE_CODE::ADD_PRINT_CONFIG):
                addPrintConfig(connection);
                break;

            case static_cast<uint8_t>(MESSAGE_CODE::REMOVE_PRINT_CONFIG):
                removePrintConfig(connection);
                break;

            case static_cast<uint8_t>(MESSAGE_CODE::CHANGE_TEMP_CONTROL):
                changeTempControl(connection);
                break;

            case static_cast<uint8_t>(MESSAGE_CODE::KEEP_ALIVE):
                // Do nothing, keep alive is done automatically.
                break;

            default:
                std::cerr << "ERROR: Invalid message code: " << static_cast<uint32_t>(connection.messageCode) << "\n";
                break;
        }

        // TODO Keep Connection alive some longer.
    }
}

void ProtoMessageHandler::sendCompletePacket(ClientConnection &connection) {
    bool tryAgain = false;
    bool wroteAnything;
    const size_t messageLength = connection.packetLength + 1u;
    size_t bytesToWrite = messageLength;

    do {
        const ssize_t bytesWritten = write(connection.socket, &connection.buffer + (messageLength - bytesToWrite), bytesToWrite);
        wroteAnything = bytesWritten > 0;

        if (!wroteAnything) {
            if (bytesWritten == EAGAIN) {
                tryAgain = true;
            } else {
                tryAgain = false;
                std::cerr << "ERROR: Connection to client has broken. This might not actually be an error (not sure).\n";
            }
        } else {
            bytesToWrite = bytesToWrite - bytesWritten;
            tryAgain = bytesToWrite != 0u;
        }

    } while (tryAgain);

    connection.connectionBroken = bytesToWrite != 0u;

    std::cout << "Message bytes sent: [";
    for (size_t index = 0u; index < messageLength; index = index + 1u) {
        std::cout << static_cast<uint32_t>(connection.buffer[index]) << ", ";
    }
    std::cout << "]\n";
}

void ProtoMessageHandler::getNextPacketLength(ClientConnection &connection) {
    receiveNBytes(connection, 1u, 0u);
    if (connection.connectionBroken) {
        std::cerr << "ERROR: Failed reading first byte of message.\n";
    } else {
        connection.packetLength = static_cast<size_t>(connection.buffer[0u]);
    }
}

void ProtoMessageHandler::receiveNBytes(ClientConnection &connection, size_t n, size_t offset) {
    bool tryAgain = false;
    size_t bytesToRead = n;

    do {
        const ssize_t bytesRead = read(connection.socket, &(connection.buffer[n - bytesToRead + offset]), bytesToRead);

        if (bytesRead < 0) {
            if (bytesRead == EAGAIN) {
                tryAgain = true;
            } else {
                tryAgain = false;
                std::cerr << "ERROR: Connection to client has broken.\n";
            }
        } else if (bytesRead == 0) {
            tryAgain = false;
            std::cerr << "ERROR: Connection to client has broken. End of file?.\n";
        } else {
            bytesToRead = bytesToRead - static_cast<size_t>(bytesRead);
            tryAgain = bytesToRead != 0u;
        }

    } while (tryAgain);

    // std::cout << "Finished receiving: bytesToRead: " << bytesToRead << " buffer: " << static_cast<uint32_t>(connection.buffer[0]) << " " << static_cast<uint32_t>(connection.buffer[1]) << " " << static_cast<uint32_t>(connection.buffer[2]) << " " << static_cast<uint32_t>(connection.buffer[3]) << "\n";
    connection.connectionBroken = bytesToRead != 0u;
}

void ProtoMessageHandler::receiveCompletePacket(ClientConnection &connection) {
    getNextPacketLength(connection);
    if (connection.connectionBroken) {
        std::cerr << "ERROR: Failed to get the packet length\n";
        return;
    }

    receiveNBytes(connection, connection.packetLength, 1u);
    if (connection.connectionBroken) {
        std::cerr << "ERROR: Failed receiving " << static_cast<uint32_t>(connection.packetLength) << " bytes.\n";
        return;
    }

    connection.messageCode = connection.buffer[connection.msgCodeOffset];
}

void ProtoMessageHandler::sendStatus(ClientConnection &connection, bool sendPrintConfigs) {
    Printer::PrinterStatus printerStatus;
    printerStatus.set_is_temp_control_active(state.tempControl);
    printerStatus.set_temperature_outside(state.outerTemp);
    printerStatus.set_temperature_inside_top(state.innerTemp);
    printerStatus.set_temperature_inside_bottom(state.innerTemp);
    Printer::PrintConfig *currentPrintConfig = new Printer::PrintConfig();
    currentPrintConfig->set_name(state.profileName);
    currentPrintConfig->set_temperature(state.profileTemp);
    printerStatus.set_allocated_current_print_config(currentPrintConfig);

    if (sendPrintConfigs || (connection.lastUpdate <= lastConfigChange)) {
        lastConfigChange = Utils::currentMillis();
        std::vector<PrintConfig> configs = PrintConfigs::getPrintConfigs();
        for (auto config = configs.begin(); config != configs.end(); config++) {
            Printer::PrintConfig *printConfig = printerStatus.add_print_configs();
            printConfig->set_name(config->name);
            printConfig->set_temperature(config->temperature);
        }
    }

    // Send protobuf (use the same buffer again).
    const size_t dataSize = printerStatus.ByteSizeLong();
    connection.packetLength = dataSize + connection.msgCodeOffset;
    if ((connection.packetLength + connection.msgDataOffset) > 256u) {
        std::cerr << "FATAL ERROR: The message size is larger than 256 bytes. The size field is only one byte long, it cannot hold more data.\n";
        connection.connectionBroken = true;
        return;
    }

    connection.buffer[0u] = static_cast<uint8_t>(connection.packetLength);
    connection.buffer[connection.msgCodeOffset] = static_cast<uint8_t>(MESSAGE_CODE::STATUS);
    if (!printerStatus.SerializeToArray(connection.buffer + connection.msgDataOffset, dataSize)) {
        std::cerr << "ERROR: Failed to serialize StatusRequest message to buffer array\n";
        connection.connectionBroken = true;
        return;
    }

    sendCompletePacket(connection);
    if (connection.connectionBroken) {
        std::cerr << "ERROR: Sending status message failed.\n";
    }
}

void ProtoMessageHandler::statusRequest(ClientConnection &connection) {
    const size_t sizeOfMsgCodeField = (connection.msgDataOffset - connection.msgCodeOffset);

    std::cout << "StatusRequest: msgLength: " + std::to_string(connection.packetLength) << "\n";

    std::cout << "Read buffer from " << connection.msgDataOffset << " to " << (connection.msgDataOffset + connection.packetLength - sizeOfMsgCodeField) << "\n";
    std::cout << "Supposed message length: " << connection.packetLength << "\n";
    std::cout << "buffer data: " << static_cast<uint32_t>(connection.buffer[2u]) << " " << static_cast<uint32_t>(connection.buffer[3u]) << "\n";

    // Read in message with protobuff.
    Printer::StatusRequest statusRequest;
    if (!statusRequest.ParseFromArray(connection.buffer + connection.msgDataOffset, connection.packetLength - sizeOfMsgCodeField)) {
        std::cerr << "ERROR: Failed to parse StatusRequest message from input array\n";
        connection.connectionBroken = true;
        return;
    }

    const bool sendPrintConfigs = statusRequest.include_print_configs();

    sendStatus(connection, sendPrintConfigs);
    if (connection.connectionBroken) {
        std::cerr << "ERROR: Failed to send status message.\n";
    }
}

void ProtoMessageHandler::addPrintConfig(ClientConnection &connection) {
    PrintConfig config;
    const size_t sizeOfMsgCodeField = (connection.msgDataOffset - connection.msgCodeOffset);

    // Read in message with protobuff.
    Printer::AddPrintConfig addPrintConfig;
    if (!addPrintConfig.ParseFromArray(connection.buffer + connection.msgDataOffset, connection.packetLength - sizeOfMsgCodeField)) {
        std::cerr << "ERROR: Failed to parse AddPrintConfig message from input array\n";
        connection.connectionBroken = true;
        return;
    }

    Printer::PrintConfig receivedConfig = addPrintConfig.print_config();
    config.name = receivedConfig.name();
    config.temperature = receivedConfig.temperature();

    // TODO inspect
    bool hasChanged = onProfileUpdateHook(config);
    if (hasChanged) {
        // Not thread safe --> There could be mistakes.
        lastConfigChange = Utils::currentMillis();
    }

    sendStatus(connection, hasChanged);
    if (connection.connectionBroken) {
        std::cerr << "ERROR: Failed to send status message.\n";
    }
}

void ProtoMessageHandler::removePrintConfig(ClientConnection &connection) {
    PrintConfig config;
    const size_t sizeOfMsgCodeField = (connection.msgDataOffset - connection.msgCodeOffset);

    // Read in message with protobuff.
    Printer::RemovePrintConfig removePrintConfig;
    if (!removePrintConfig.ParseFromArray(connection.buffer + connection.msgDataOffset, connection.packetLength - sizeOfMsgCodeField)) {
        std::cerr << "ERROR: Failed to parse RemovePrintConfig message from input array\n";
        connection.connectionBroken = true;
        return;
    }

    Printer::PrintConfig receivedConfig = removePrintConfig.print_config();
    config.name = receivedConfig.name();
    config.temperature = receivedConfig.temperature();

    bool hasChanged = PrintConfigs::removeConfig(config, state);
    if (hasChanged) {
        lastConfigChange = Utils::currentMillis();
    }
    sendStatus(connection, hasChanged);
    if (connection.connectionBroken) {
        std::cerr << "ERROR: Failed to send status message.\n";
    }
}

void ProtoMessageHandler::changeTempControl(ClientConnection &connection) {
    const size_t sizeOfMsgCodeField = (connection.msgDataOffset - connection.msgCodeOffset);

    // Read in message with protobuff.
    Printer::ChangeTempControl changeTempControl;
    if (!changeTempControl.ParseFromArray(connection.buffer + connection.msgDataOffset, connection.packetLength - sizeOfMsgCodeField)) {
        std::cerr << "ERROR: Failed to parse ChangeTempControl message from input array\n";
        connection.connectionBroken = true;
        return;
    }

    // Change temp control
    tempControlChangeHook(changeTempControl.is_active());

    sendStatus(connection, false);
    if (connection.connectionBroken) {
        std::cerr << "ERROR: Failed to send status message.\n";
    }
}

void ProtoMessageHandler::updateState(PrinterState &state) {
    this->state = state;
}