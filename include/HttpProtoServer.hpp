#pragma once

#include <functional>
#include <string>

#include "PrintConfigs.hpp"
#include "PrinterData.pb.h"
#include "PrinterState.hpp"
#include "uWebSockets/App.h"

#define HOST_PORT 1933

class HttpProtoServer {
   private:
    uWS::App app = uWS::App();
    PrinterState state;
    std::function<void(void)> shutdownHook;
    std::function<bool(PrintConfig &)> onProfileUpdateHook;
    std::function<void(bool)> tempControlChangeHook;
    std::function<void(void)> onActivity;

    bool statusRequest(std::vector<char> buffer, uWS::HttpResponse<false> *res);
    bool addPrintConfig(std::vector<char> buffer, uWS::HttpResponse<false> *res);
    bool removePrintConfig(std::vector<char> buffer, uWS::HttpResponse<false> *res);
    bool changeTempControl(std::vector<char> buffer, uWS::HttpResponse<false> *res);
    bool sendStatus(bool sendPrintConfigs, uWS::HttpResponse<false> *res);
    void handleHttpRequest(uWS::HttpResponse<false> *res, uWS::HttpRequest *req, std::function<bool(std::vector<char> buffer)> parser);
    void addCorsHeaders(uWS::HttpResponse<false> *res);

    void startHttpServer();

   public:
    bool start();
    void updateState(PrinterState &state);

    HttpProtoServer(std::function<void(void)> shutdownHook, std::function<bool(PrintConfig &)> onProfileUpdate, std::function<void(bool)> onTempControlChange, std::function<void(void)> onActivity);
    ~HttpProtoServer();
};