#include "HttpProtoServer.hpp"

#include <iostream>

HttpProtoServer::HttpProtoServer(PrinterState &state, const uint16_t port, const std::function<void(void)> shutdownHook, const std::function<bool(PrintConfig &)> onProfileUpdate, const std::function<void(void)> onActivity) : state(state), port(port), onShutdown(shutdownHook), onProfileUpdate(onProfileUpdate), onActivity(onActivity) {}

void HttpProtoServer::start() {
    startHttpServer();
}

void HttpProtoServer::startHttpServer() {
    app.addServerName("Printer-3D");
    app.listen(port, [this](auto *listen_socket) {
        if (listen_socket) {
            std::cout << "Listening on port " << port << std::endl;
        } else {
            std::cout << "Thread " << std::this_thread::get_id() << " failed to listen on port " << port << std::endl;
        }
    });

    app.post("/statusRequest", [this](auto *res, auto *req) mutable {
        handleHttpRequest(res, req, [this, res](auto data) mutable {
            return statusRequest(data, res);
        });
    });

    app.post("/addPrintConfig", [this](auto *res, auto *req) mutable {
        handleHttpRequest(res, req, [this, res](auto data) mutable {
            return addPrintConfig(data, res);
        });
    });

    app.post("/removePrintConfig", [this](auto *res, auto *req) mutable {
        handleHttpRequest(res, req, [this, res](auto data) mutable {
            return removePrintConfig(data, res);
        });
    });

    app.post("/changeTempControl", [this](auto *res, auto *req) mutable {
        handleHttpRequest(res, req, [this, res](auto data) mutable {
            return changeTempControl(data, res);
        });
    });

    app.post("/shutdown", [this](auto *res, auto *req) mutable {
        addCorsHeaders(res);
        res->endWithoutBody();
        onShutdown();
    });

    app.run();
}

void HttpProtoServer::handleHttpRequest(uWS::HttpResponse<false> *res, uWS::HttpRequest *req, std::function<bool(std::vector<char> buffer)> parser) {
    bool hasAborted = false;
    std::vector<char> buffer;

    res->onAborted([&hasAborted]() mutable { hasAborted = true; std::cout << "Aborted\n"; });

    res->onData([this, res, hasAborted, buffer, parser](std::string_view data, bool isAll) mutable {
        if (hasAborted)
            return;

        buffer.insert(buffer.begin(), data.begin(), data.end());

        if (isAll) {
            if (!parser(buffer)) {
                std::cerr << "ERROR: Failed processing the received data\n";
                res->writeStatus(std::string("500 Internal Server Error"));
                addCorsHeaders(res);
                res->end(std::string("{\"code\":500,\"message\":\"Internal Server Error. Something failed processing the given data.\"}"));
            }
        }
    });

    onActivity();
}

void HttpProtoServer::sendStatus(bool sendPrintConfigs, uWS::HttpResponse<false> *res) {
    Printer::PrinterStatus printerStatus;
    printerStatus.set_is_temp_control_active(state.getIsTempControlActive());
    printerStatus.set_temperature_outside(state.getOuterTemp());
    printerStatus.set_temperature_inside_top(state.getInnerTopTemp());
    printerStatus.set_temperature_inside_bottom(state.getInnerBottomTemp());
    printerStatus.set_fan_speed(state.getFanSpeed());
    Printer::PrintConfig *currentPrintConfig = new Printer::PrintConfig();
    currentPrintConfig->set_name(state.getProfileName());
    currentPrintConfig->set_temperature(state.getProfileTemp());
    printerStatus.set_allocated_current_print_config(currentPrintConfig);
    printerStatus.set_fan_speed(state.getFanSpeed());

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
    addCorsHeaders(res);
    res->end(outData);
}

void HttpProtoServer::addCorsHeaders(uWS::HttpResponse<false> *res) {
    res->writeHeader("Access-Control-Allow-Origin", "*");
    res->writeHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    res->writeHeader("Access-Control-Allow-Headers", "origin, content-type, accept, x-requested-with");
}

bool HttpProtoServer::statusRequest(std::vector<char> buffer, uWS::HttpResponse<false> *res) {
    Printer::StatusRequest statusRequest;

    if (!statusRequest.ParseFromArray(buffer.data(), buffer.size())) {
        std::cerr << "ERROR: Failed to parse StatusRequest message from input array\n";
        return false;
    }

    const bool sendPrintConfigs = statusRequest.include_print_configs();

    sendStatus(sendPrintConfigs, res);
    return true;
}

bool HttpProtoServer::addPrintConfig(std::vector<char> buffer, uWS::HttpResponse<false> *res) {
    PrintConfig config;

    // Read in message with protobuff.
    Printer::AddPrintConfig addPrintConfig;
    if (!addPrintConfig.ParseFromArray(buffer.data(), buffer.size())) {
        std::cerr << "ERROR: Failed to parse AddPrintConfig message from input array\n";
        return false;
    }

    Printer::PrintConfig receivedConfig = addPrintConfig.print_config();
    config.name = receivedConfig.name();
    config.temperature = receivedConfig.temperature();
    const bool hasChanged = onProfileUpdate(config);

    sendStatus(hasChanged, res);

    if (hasChanged) {
        state.setProfileTemp(config.temperature, false);
        state.setProfileName(config.name, true);
    }

    return true;
}

bool HttpProtoServer::removePrintConfig(std::vector<char> buffer, uWS::HttpResponse<false> *res) {
    PrintConfig config;

    // Read in message with protobuff.
    Printer::RemovePrintConfig removePrintConfig;
    if (!removePrintConfig.ParseFromArray(buffer.data(), buffer.size())) {
        std::cerr << "ERROR: Failed to parse RemovePrintConfig message from input array\n";
        return false;
    }

    Printer::PrintConfig receivedConfig = removePrintConfig.print_config();
    config.name = receivedConfig.name();
    config.temperature = receivedConfig.temperature();
    const bool hasChanged = PrintConfigs::removeConfig(config, state);

    sendStatus(hasChanged, res);

    if (hasChanged) {
        state.setProfileTemp(config.temperature, false);
        state.setProfileName(config.name, true);
    }

    return true;
}

bool HttpProtoServer::changeTempControl(std::vector<char> buffer, uWS::HttpResponse<false> *res) {
    // Read in message with protobuff.
    Printer::ChangeTempControl changeTempControl;
    if (!changeTempControl.ParseFromArray(buffer.data(), buffer.size())) {
        std::cerr << "ERROR: Failed to parse ChangeTempControl message from input array\n";
        return false;
    }

    PrintConfig config;
    config.name = changeTempControl.selected_print_config().name();
    config.temperature = changeTempControl.selected_print_config().temperature();
    const bool hasChanged = onProfileUpdate(config);

    sendStatus(false, res);

    if (hasChanged) {
        state.setProfileTemp(config.temperature, false);
        state.setProfileName(config.name, false);
    }

    state.setIsTempControlActive(changeTempControl.is_active(), true);
    return true;
}