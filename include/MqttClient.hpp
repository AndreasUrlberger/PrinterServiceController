#pragma once

#include <mosquittopp.h>
#include <string>
#include "PrinterState.hpp"

class MqttClient : public mosqpp::mosquittopp {

private:
    std::string clientId_;
    std::string brokerIp_;
    int port_;
    PrinterState &state_;

public:
    MqttClient(const std::string &clientId, const std::string &brokerIp, const int port, PrinterState &state);
    ~MqttClient();
    void on_connect(int rc) override;
    void on_message(const struct mosquitto_message *message) override;

    void publishCurrentState();
    void publishProfiles();
    void toggleTempControl(const std::string &payload);
};