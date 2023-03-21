#include "WsProtoHandler.h"

#include <iostream>

WsProtoHandler::WsProtoHandler(std::function<void(void)> shutdownHook, std::function<bool(PrintConfig &)> onProfileUpdate, std::function<void(bool)> onTempControlChange) {
    this->shutdownHook = shutdownHook;
    onProfileUpdateHook = onProfileUpdate;
    tempControlChangeHook = onTempControlChange;
}

WsProtoHandler::~WsProtoHandler() {}

bool WsProtoHandler::start() {
    startWsServer();
    return true;
}

void WsProtoHandler::startWsServer() {
    std::cout << "Called startWsServer\n";
    behaviour.compression = uWS::DISABLED,
    behaviour.maxPayloadLength = 16 * 1024, behaviour.idleTimeout = 120,
    behaviour.closeOnBackpressureLimit = false,
    behaviour.resetIdleTimeoutOnSend = true,
    behaviour.upgrade = [](uWS::HttpResponse<false> *res, uWS::HttpRequest *req, us_socket_context_t *context) {
        std::cout << "UPGRADE\n";
    };
    behaviour.open = [](auto *ws) {
        std::cout << "OPEN\n";
    };
    behaviour.message = [this](
                            uWS::WebSocket<false, true, ClientConnection> *ws,
                            std::string_view message,
                            uWS::OpCode opCode) {
        std::cout << "MESSAGE\n";
    };
    behaviour.close = [](auto *ws, int code, std::string_view message) {
        std::cout << "CLOSE\n";
    };
    behaviour.subscription = [](uWS::WebSocket<false, true, WsProtoHandler::ClientConnection> *, std::string_view, int, int) {
        std::cout << "SUBSCRIPTION\n";
    };

    app.addServerName("Printer-3D");
    app.listen(HOST_PORT, [](auto *listen_socket) {
        if (listen_socket) {
            std::cout << "Listening on port " << HOST_PORT << std::endl;
        } else {
            std::cout << "Thread " << std::this_thread::get_id() << " failed to listen on port " << HOST_PORT << std::endl;
        }
    });

    // app.get("/statusRequest", [this](uWS::HttpResponse<false> *res, uWS::HttpRequest *req) mutable {
    //     onStatusRequest(res, req, [this](auto data, auto *res) mutable { return statusRequest(data, res); });
    // });

    app.get("/statusRequest", [this](uWS::HttpResponse<false> *res, uWS::HttpRequest *req) mutable {
        // TODO Should set some abort variable.
        res->onAborted([]() { std::cout << "Aborted\n"; });

        res->onData([this, res](std::string_view data, bool isAll) mutable {
            std::cout << "Received data: " << data << "\n";

            if (isAll) {
                std::string outData = statusRequest(data);
                // std::string outData = "";
                if (outData.empty()) {
                    std::cerr << "ERROR: Failed processing the received data\n";
                    res->writeStatus(std::string("500 Internal Server Error"));
                    res->end(std::string("{\"code\":500,\"message\":\"Internal Server Error. Something failed processing the given data.\"}"));
                } else {
                    std::cout << "Final Outdata: " << outData << "\n";
                    res->end(outData);
                }
            } else {
                std::cerr << "ERROR: Request data is too long\n";
                res->writeStatus(std::string("413 Payload Too Large"));
                res->end(std::string("{\"code\":413,\"message\":\"Payload Too Large\"}"));
            }
        });
    });

    std::cout << "Next step is running the app\n";

    app.run();

    std::cout << "Ran the app\n";
}

std::string WsProtoHandler::onStatusRequest(uWS::HttpResponse<false> *res, uWS::HttpRequest *req, std::function<std::string(std::string_view)> parser) {
    // TODO Should set some abort variable.
    res->onAborted([]() { std::cout << "Aborted\n"; });

    res->onData([&](std::string_view data, bool isAll) {
        std::cout << "Received data: " << data << "\n";

        return;

        if (isAll) {
            // std::string outData = statusRequest(data);
            std::string outData = "";
            if (outData.empty()) {
                std::cerr << "ERROR: Failed processing the received data\n";

                std::cout << "Call writeStatus next\n";
                res->writeStatus(std::string("500 Internal Server Error"));
                std::cout << "Call end next\n";
                res->end(std::string("{\"code\":500,\"message\":\"Internal Server Error. Something failed processing the given data.\"}"));
            } else {
                res->end(outData);
            }
        } else {
            std::cerr << "ERROR: Request data is too long\n";
            res->writeStatus(std::string("413 Payload Too Large"));
            res->end(std::string("{\"code\":413,\"message\":\"Payload Too Large\"}"));
        }
    });
}

std::string WsProtoHandler::sendStatus(bool sendPrintConfigs) {
    Printer::PrinterStatus printerStatus;
    printerStatus.set_is_temp_control_active(state.tempControl);
    printerStatus.set_temperature_outside(state.outerTemp);
    printerStatus.set_temperature_inside_top(state.innerTemp);
    printerStatus.set_temperature_inside_bottom(state.innerTemp);
    Printer::PrintConfig *currentPrintConfig = new Printer::PrintConfig();
    currentPrintConfig->set_name(state.profileName);
    currentPrintConfig->set_temperature(state.profileTemp);
    printerStatus.set_allocated_current_print_config(currentPrintConfig);

    if (sendPrintConfigs) {
        lastConfigChange = Utils::currentMillis();
        std::vector<PrintConfig> configs = PrintConfigs::getPrintConfigs();
        for (auto config = configs.begin(); config != configs.end(); config++) {
            Printer::PrintConfig *printConfig =
                printerStatus.add_print_configs();
            printConfig->set_name(config->name);
            printConfig->set_temperature(config->temperature);
        }
    }

    const std::string outData = printerStatus.SerializeAsString();
    return outData;
}

std::string WsProtoHandler::statusRequest(std::string_view data) {
    std::cout << "Data bytes received, length: " << data.length() << " : [";
    for (size_t index = 0u; index < data.length(); index = index + 1u) {
        std::cout << static_cast<uint32_t>(data[index]) << ", ";
    }
    std::cout << "]\n";

    Printer::StatusRequest statusRequest;
    if (!statusRequest.ParseFromArray(data.begin(), data.length())) {
        std::cerr << "ERROR: Failed to parse StatusRequest message from input array\n";
        return "";
    }

    const bool sendPrintConfigs = statusRequest.include_print_configs();

    std::cout << "SendPrintConfigs: " << (sendPrintConfigs ? "yes" : "no") << "\n";

    return sendStatus(sendPrintConfigs);
}

bool WsProtoHandler::addPrintConfig(ClientConnection &connection, std::string_view const message) {
    PrintConfig config;

    // Read in message with protobuff.
    Printer::AddPrintConfig addPrintConfig;
    if (!addPrintConfig.ParseFromArray(message.begin(), message.length())) {
        std::cerr << "ERROR: Failed to parse AddPrintConfig message from input array\n";
        return false;
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

    // if (sendStatus(connection, hasChanged)) {
    //     return true;
    // } else {
    //     std::cerr << "ERROR: Failed to send status message.\n";
    //     return false;
    // }
    return false;
}

bool WsProtoHandler::removePrintConfig(ClientConnection &connection, std::string_view const message) {
    PrintConfig config;

    // Read in message with protobuff.
    Printer::RemovePrintConfig removePrintConfig;
    if (!removePrintConfig.ParseFromArray(message.begin(), message.length())) {
        std::cerr << "ERROR: Failed to parse RemovePrintConfig message from input array\n";
        return false;
    }

    Printer::PrintConfig receivedConfig = removePrintConfig.print_config();
    config.name = receivedConfig.name();
    config.temperature = receivedConfig.temperature();

    bool hasChanged = PrintConfigs::removeConfig(config);
    if (hasChanged) {
        lastConfigChange = Utils::currentMillis();
    }

    // if (sendStatus(connection, hasChanged)) {
    //     return true;
    // } else {
    //     std::cerr << "ERROR: Failed to send status message.\n";
    //     return false;
    // }
    return false;
}

bool WsProtoHandler::changeTempControl(ClientConnection &connection, std::string_view const message) {
    // Read in message with protobuff.
    Printer::ChangeTempControl changeTempControl;
    if (!changeTempControl.ParseFromArray(message.begin(), message.length())) {
        std::cerr << "ERROR: Failed to parse ChangeTempControl message from input array\n";
        return false;
    }

    // Change temp control
    tempControlChangeHook(changeTempControl.isactive());

    // if (sendStatus(connection, false)) {
    //     return true;
    // } else {
    //     std::cerr << "ERROR: Failed to send status message.\n";
    //     return false;
    // }
    return false;
}

void WsProtoHandler::updateState(PrinterState &state) { this->state = state; }