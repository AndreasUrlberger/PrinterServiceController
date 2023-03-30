#pragma once

#include <functional>
#include <string>

#include "PrintConfigs.hpp"
#include "PrinterData.pb.h"
#include "PrinterState.hpp"
#include "uWebSockets/App.h"

class HttpProtoServer {
   private:
    // PRIVATE VARIABLES.
    uWS::App app{};
    PrinterState &state;
    const uint16_t port;
    const std::function<void(void)> onShutdown;
    const std::function<bool(PrintConfig &)> onProfileUpdate;
    const std::function<void(bool)> onTempControlChange;
    const std::function<void(void)> onActivity;

    // PRIVATE FUNCTIONS.
    bool statusRequest(std::vector<char> buffer, uWS::HttpResponse<false> *res);
    bool addPrintConfig(std::vector<char> buffer, uWS::HttpResponse<false> *res);
    bool removePrintConfig(std::vector<char> buffer, uWS::HttpResponse<false> *res);
    bool changeTempControl(std::vector<char> buffer, uWS::HttpResponse<false> *res);
    void sendStatus(bool sendPrintConfigs, uWS::HttpResponse<false> *res);
    void handleHttpRequest(uWS::HttpResponse<false> *res, uWS::HttpRequest *req, std::function<bool(std::vector<char> buffer)> parser);
    void addCorsHeaders(uWS::HttpResponse<false> *res);

    void startHttpServer();

   public:
    // PUBLIC FUNCTIONS.
    void start();

    HttpProtoServer(PrinterState &state, const uint16_t port, const std::function<void(void)> onShutdown, const std::function<bool(PrintConfig &)> onProfileUpdate, const std::function<void(void)> onActivity);
    ~HttpProtoServer() = default;
};