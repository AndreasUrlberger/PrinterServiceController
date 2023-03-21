#include "HttpProtoServer.h"

#include <iostream>

HttpProtoServer::HttpProtoServer(std::function<void(void)> shutdownHook, std::function<bool(PrintConfig &)> onProfileUpdate, std::function<void(bool)> onTempControlChange) {
    this->shutdownHook = shutdownHook;
    onProfileUpdateHook = onProfileUpdate;
    tempControlChangeHook = onTempControlChange;
}

HttpProtoServer::~HttpProtoServer() {}

bool HttpProtoServer::start() {
    startHttpServer();
    return true;
}

void HttpProtoServer::startHttpServer() {
    std::cout << "start http server\n";

    app.addServerName("Printer-3D");
    app.listen(HOST_PORT, [](auto *listen_socket) {
        if (listen_socket) {
            std::cout << "Listening on port " << HOST_PORT << std::endl;
        } else {
            std::cout << "Thread " << std::this_thread::get_id() << " failed to listen on port " << HOST_PORT << std::endl;
        }
    });

    app.get("/statusRequest", [this](auto *res, auto *req) mutable {
        handleHttpRequest(res, req, [this, res](auto data) mutable {
            return statusRequest(data, res);
        });
    });

    app.get("/addPrintConfig", [this](auto *res, auto *req) mutable {
        handleHttpRequest(res, req, [this, res](auto data) mutable {
            return addPrintConfig(data, res);
        });
    });

    app.get("/removePrintConfig", [this](auto *res, auto *req) mutable {
        handleHttpRequest(res, req, [this, res](auto data) mutable {
            return removePrintConfig(data, res);
        });
    });

    app.get("/changeTempControl", [this](auto *res, auto *req) mutable {
        handleHttpRequest(res, req, [this, res](auto data) mutable {
            return changeTempControl(data, res);
        });
    });

    app.run();
}

void HttpProtoServer::handleHttpRequest(uWS::HttpResponse<false> *res, uWS::HttpRequest *req, std::function<bool(std::string_view)> parser) {
    bool hasAborted = false;
    res->onAborted([&hasAborted]() mutable { hasAborted = true; std::cout << "Aborted\n"; });

    res->onData([this, res, hasAborted](std::string_view data, bool isAll) mutable {
        if (hasAborted)
            return;

        if (isAll) {
            if (!statusRequest(data, res)) {
                std::cerr << "ERROR: Failed processing the received data\n";
                res->writeStatus(std::string("500 Internal Server Error"));
                res->end(std::string("{\"code\":500,\"message\":\"Internal Server Error. Something failed processing the given data.\"}"));
            }
        } else {
            std::cerr << "ERROR: Request data is too long\n";
            res->writeStatus(std::string("413 Payload Too Large"));
            res->end(std::string("{\"code\":413,\"message\":\"Payload Too Large\"}"));
        }
    });
}

bool HttpProtoServer::sendStatus(bool sendPrintConfigs, uWS::HttpResponse<false> *res) {
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
        std::vector<PrintConfig> configs = PrintConfigs::getPrintConfigs();
        for (auto config = configs.begin(); config != configs.end(); config++) {
            Printer::PrintConfig *printConfig =
                printerStatus.add_print_configs();
            printConfig->set_name(config->name);
            printConfig->set_temperature(config->temperature);
        }
    }

    const std::string outData = printerStatus.SerializeAsString();
    res->end(outData);
    return true;
}

bool HttpProtoServer::statusRequest(std::string_view data, uWS::HttpResponse<false> *res) {
    Printer::StatusRequest statusRequest;
    if (!statusRequest.ParseFromArray(data.begin(), data.length())) {
        std::cerr << "ERROR: Failed to parse StatusRequest message from input array\n";
        return false;
    }

    const bool sendPrintConfigs = statusRequest.include_print_configs();

    return sendStatus(sendPrintConfigs, res);
}

bool HttpProtoServer::addPrintConfig(std::string_view data, uWS::HttpResponse<false> *res) {
    PrintConfig config;

    // Read in message with protobuff.
    Printer::AddPrintConfig addPrintConfig;
    if (!addPrintConfig.ParseFromArray(data.begin(), data.length())) {
        std::cerr << "ERROR: Failed to parse AddPrintConfig message from input array\n";
        return false;
    }

    Printer::PrintConfig receivedConfig = addPrintConfig.print_config();
    config.name = receivedConfig.name();
    config.temperature = receivedConfig.temperature();

    const bool hasChanged = onProfileUpdateHook(config);

    return sendStatus(hasChanged, res);
}

bool HttpProtoServer::removePrintConfig(std::string_view data, uWS::HttpResponse<false> *res) {
    PrintConfig config;

    // Read in message with protobuff.
    Printer::RemovePrintConfig removePrintConfig;
    if (!removePrintConfig.ParseFromArray(data.begin(), data.length())) {
        std::cerr << "ERROR: Failed to parse RemovePrintConfig message from input array\n";
        return false;
    }

    Printer::PrintConfig receivedConfig = removePrintConfig.print_config();
    config.name = receivedConfig.name();
    config.temperature = receivedConfig.temperature();

    const bool hasChanged = PrintConfigs::removeConfig(config);

    return sendStatus(hasChanged, res);
}

bool HttpProtoServer::changeTempControl(std::string_view data, uWS::HttpResponse<false> *res) {
    // Read in message with protobuff.
    Printer::ChangeTempControl changeTempControl;
    if (!changeTempControl.ParseFromArray(data.begin(), data.length())) {
        std::cerr << "ERROR: Failed to parse ChangeTempControl message from input array\n";
        return false;
    }

    // Change temp control
    tempControlChangeHook(changeTempControl.isactive());

    return sendStatus(false, res);
}

void HttpProtoServer::updateState(PrinterState &state) { this->state = state; }