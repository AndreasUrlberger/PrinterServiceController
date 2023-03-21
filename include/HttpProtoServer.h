#pragma once

#include <functional>
#include <string>

#include "PrintConfigs.h"
#include "PrinterData.pb.h"
#include "Utils.h"
#include "uWebSockets/App.h"

#define HOST_PORT 1933

class HttpProtoServer {
    enum MESSAGE_CODE {
        STATUS_REQUEST = 0,
        ADD_PRINT_CONFIG = 1,
        REMOVE_PRINT_CONFIG = 2,
        CHANGE_TEMP_CONTROL = 3,
        KEEP_ALIVE = 4,
        STATUS = 5
    };

   private:
    uWS::App app = uWS::App();
    PrinterState state;
    std::function<void(void)> shutdownHook;
    std::function<bool(PrintConfig &)> onProfileUpdateHook;
    std::function<void(bool)> tempControlChangeHook;

    bool statusRequest(std::string_view data, uWS::HttpResponse<false> *res);
    bool addPrintConfig(std::string_view data, uWS::HttpResponse<false> *res);
    bool removePrintConfig(std::string_view data, uWS::HttpResponse<false> *res);
    bool changeTempControl(std::string_view data, uWS::HttpResponse<false> *res);
    bool sendStatus(bool sendPrintConfigs, uWS::HttpResponse<false> *res);
    void handleHttpRequest(uWS::HttpResponse<false> *res, uWS::HttpRequest *req, std::function<bool(std::string_view)> parser);

    void startHttpServer();

   public:
    bool start();
    void updateState(PrinterState &state);

    HttpProtoServer(std::function<void(void)> shutdownHook, std::function<bool(PrintConfig &)> onProfileUpdate, std::function<void(bool)> onTempControlChange);
    ~HttpProtoServer();
};