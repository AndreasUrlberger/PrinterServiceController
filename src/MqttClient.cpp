#include "MqttClient.hpp"
#include <iostream>
#include "json/json.hpp"
#include "PrintConfigs.hpp"

// Constructor definition
MqttClient::MqttClient(const std::string &clientId, const std::string &brokerIp, const int port, PrinterState &state)
    : mosqpp::mosquittopp(clientId.c_str()), clientId_(clientId), brokerIp_(brokerIp), port_(port), state_(state) {
    std::cout << "Connecting to MQTT Broker at: " << brokerIp_ << ":" << port_ << "\n";

    loop_start();

    int connectionResult = connect(brokerIp_.c_str(), port_);
    if (connectionResult != MOSQ_ERR_SUCCESS) {
        std::cerr << "Failed to connect to MQTT Broker, return code: " << connectionResult << "\n";
    }else{
        std::cout << "Connection to MQTT Broker successful!\n";
    }
}

MqttClient::~MqttClient() {
    loop_stop();
}

// on_connect method definition
void MqttClient::on_connect(int rc) {
    if (rc == 0) {
        subscribe(nullptr, "printer/#");
        std::cout << "Connected to MQTT Broker!\n";
    } else {
        std::cerr << "Failed to connect, return code: " << rc << "\n";
    }
}

// on_message method definition
void MqttClient::on_message(const struct mosquitto_message *message) {
    const std::string payload = std::string(static_cast<char *>(message->payload), message->payloadlen);
    const std::string topic = message->topic;

    if (message->topic == std::string("printer/command")) {
        std::cout << "printer/command received: " << payload << "\n";
        publishCurrentState();
    } else if (message->topic == std::string("printer/get_profiles")) {
        std::cout << "printer/get_profiles received: " << payload << "\n";
        publishProfiles();
    } else if (message->topic == std::string("printer/toggle_temp_control")) {
        std::cout << "printer/toggle_temp_control received: " << payload << "\n";
        toggleTempControl(payload);
    } else {
        std::cout << "Unknown topic: " << message->topic << "\n";
    }
}

void MqttClient::publishCurrentState() {
    // Create json containing the current state
    nlohmann::json stateJson;
    stateJson["isTempControlActive"] = state_.getIsTempControlActive();
    stateJson["outerTemp"] = state_.getOuterTemp();
    stateJson["innerTopTemp"] = state_.getInnerTopTemp();
    stateJson["innerBottomTemp"] = state_.getInnerBottomTemp();
    stateJson["fanSpeed"] = state_.getFanSpeed();
    stateJson["profileName"] = state_.getProfileName();
    stateJson["profileTemp"] = state_.getProfileTemp();

    const std::string payload = stateJson.dump();
    const std::string topic = "printer/status";
    publish(nullptr, topic.c_str(), payload.size(), payload.c_str());
    std::cout << "Current state sent\n";
}

void MqttClient::publishProfiles() {
    // Create json array containing the profiles
    nlohmann::json profilesJson = nlohmann::json::array();
    
    // Add profiles to the json
    std::vector<PrintConfig> configs = PrintConfigs::getPrintConfigs();
    for (auto config = configs.begin(); config != configs.end(); config++) {
        nlohmann::json profileJson;
        profileJson["name"] = config->name;
        profileJson["temperature"] = config->temperature;
        profilesJson.push_back(profileJson);
    }

    const std::string payload = profilesJson.dump();
    const std::string topic = "printer/profiles";
    publish(nullptr, topic.c_str(), payload.size(), payload.c_str());
    std::cout << "Profiles sent\n";

    // Send it here as well since this is the only topic that is triggered frequently by the client.
    sendTempControlState();
}

void MqttClient::toggleTempControl(const std::string &payload) {
    if (payload == "on") {
        state_.setIsTempControlActive(true, true);
    } else if (payload == "off") {
        state_.setIsTempControlActive(false, true);
    } else {
        std::cerr << "Invalid payload for toggleTempControl: " << payload << "\n";
    }

    sendTempControlState();
}

void MqttClient::sendTempControlState() {
    const std::string payload = state_.getIsTempControlActive() ? "on" : "off";
    const std::string topic = "printer/temp_control";
    publish(nullptr, topic.c_str(), payload.size(), payload.c_str());
    std::cout << "Temp control state sent\n";
}