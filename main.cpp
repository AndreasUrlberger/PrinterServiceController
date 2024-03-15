#include "Logger.hpp"
#include "ServiceController.hpp"

#include "json/json.hpp"
#include <fstream>
#include <iostream>

template <typename T>
T getOrDefault(const nlohmann::json &json, const std::string &key, const T &defaultValue);

ServiceController *createController(nlohmann::json &config);

bool isValidSubconfig(const nlohmann::json &subconfig, const std::string key);

int main(int argc, char *argv[])
{
    Logger::enableLogging();

    nlohmann::json config;
    std::ifstream configFile("./PrinterConfig.json");
    configFile >> config;

    ServiceController *controller = createController(config);
    controller->run();
}

template <typename T>
T getOrDefault(const nlohmann::json &json, const std::string &key, const T &defaultValue)
{
    if (json.find(key) == json.end())
    {
        std::cout << "Key '" << key << "' not found, using default value: '" << defaultValue << "'\n";
        return defaultValue;
    }
    else
    {
        const nlohmann::json json_value = json[key];
        return json_value.get<T>();
    }
}

bool isValidSubconfig(const nlohmann::json &subconfig, const std::string key)
{
    if (subconfig.is_null())
    {
        std::cout << "Config is missing subcategory: " << key << "\n";
        return false;
    }
    else
    {
        return true;
    }
}

ServiceController *createController(nlohmann::json &config)
{
    nlohmann::json thermoConfigJson = config["thermometer_config"];
    if (!isValidSubconfig(thermoConfigJson, "thermometer_config"))
    {
        thermoConfigJson = nlohmann::json::object();
    }
    const ServiceController::ThermometerConfig thermoConfig{
        getOrDefault<std::string>(thermoConfigJson, "inner_top_thermo_name", "28-2ca0a72153ff"),
        getOrDefault<std::string>(thermoConfigJson, "inner_bottom_thermo_name", "28-3c290457da46"),
        getOrDefault<std::string>(thermoConfigJson, "outer_thermo_name", "28-baa0a72915ff")};

    nlohmann::json generalConfigJson = config["general_config"];
    if (!isValidSubconfig(generalConfigJson, "general_config"))
    {
        generalConfigJson = nlohmann::json::object();
    }
    const ServiceController::GeneralConfig generalConfig{
        getOrDefault<uint64_t>(generalConfigJson, "screen_alive_time_ms", 20'000),
        getOrDefault<uint64_t>(generalConfigJson, "max_inactive_time_ms", 3'000)};

    nlohmann::json lightConfigJson = config["light_config"];
    if (!isValidSubconfig(lightConfigJson, "light_config"))
    {
        lightConfigJson = nlohmann::json::object();
    }
    const ServiceController::LightConfig lightConfig{
        getOrDefault<std::string>(lightConfigJson, "bridge_ip", "192.168.178.92"),
        getOrDefault<std::string>(lightConfigJson, "bridge_username", "yXEXyXC5BYDJbmS-6yNdji6qcatY6BedJmRIb4kO"),
        getOrDefault<uint8_t>(lightConfigJson, "printer_light_id", 9)};

    nlohmann::json displayConfigJson = config["display_config"];
    if (!isValidSubconfig(displayConfigJson, "display_config"))
    {
        displayConfigJson = nlohmann::json::object();
    }
    const ServiceController::DisplayConfig displayConfig{
        getOrDefault<uint8_t>(displayConfigJson, "height", 64),
        getOrDefault<uint8_t>(displayConfigJson, "width", 128),
        getOrDefault<std::string>(displayConfigJson, "file_name", "/dev/i2c-3"),
        getOrDefault<uint8_t>(displayConfigJson, "font_size", 5),
        getOrDefault<std::string>(displayConfigJson, "font_name", "/usr/share/fonts/truetype/freefont/FreeMono.ttf")};

    nlohmann::json httpServerConfigJson = config["http_server_config"];
    if (!isValidSubconfig(httpServerConfigJson, "http_server_config"))
    {
        httpServerConfigJson = nlohmann::json::object();
    }
    const ServiceController::HttpServerConfig httpServerConfig{getOrDefault<uint16_t>(httpServerConfigJson, "port", 1933)};

    nlohmann::json fanConfigJson = config["fan_controller_config"];
    if (!isValidSubconfig(fanConfigJson, "fan_controller_config"))
    {
        fanConfigJson = nlohmann::json::object();
    }
    FanController::FanControllerConfig fanConfig{
        getOrDefault<uint8_t>(fanConfigJson, "pwm_pin", 12),
        getOrDefault<uint8_t>(fanConfigJson, "tick_pin", 7),
        getOrDefault<uint8_t>(fanConfigJson, "relay_pin", 11),
        getOrDefault<uint8_t>(fanConfigJson, "led_pin", 23),
        getOrDefault<float>(fanConfigJson, "min_rpm", 750.0f),
        getOrDefault<float>(fanConfigJson, "max_rpm", 3'000.0f),
        getOrDefault<uint64_t>(fanConfigJson, "speed_meas_period_ms", 3'000),
        getOrDefault<float>(fanConfigJson, "temp_meas_period_s", 1.0f),
        getOrDefault<float>(fanConfigJson, "min_integral", -5'000.0f),
        getOrDefault<float>(fanConfigJson, "max_integral", 5'000.0f),
        getOrDefault<float>(fanConfigJson, "max_temp_deviation", 1.0f),
        getOrDefault<float>(fanConfigJson, "proportional_factor", 10.0f),
        getOrDefault<float>(fanConfigJson, "integral_factor", 10.0f),
        getOrDefault<uint64_t>(fanConfigJson, "blink_interval_ms", 500)};

    nlohmann::json cameraConfigJson = config["camera_config"];
    if (!isValidSubconfig(cameraConfigJson, "camera_config"))
    {
        cameraConfigJson = nlohmann::json::object();
    }
    ServiceController::CameraConfig cameraConfig{
        getOrDefault<std::string>(cameraConfigJson, "start_command", "nohup mjpg_streamer -o \"output_http.so -w ./www -p 8000\" -i \"input_uvc.so -r 1280x720 -f 1 -softfps 1\" > /dev/null 2>&1 &"),
        getOrDefault<std::string>(cameraConfigJson, "stop_command", "kill $(pidof mjpg_streamer)")};

    return new ServiceController{thermoConfig, generalConfig, cameraConfig, lightConfig, displayConfig, httpServerConfig, fanConfig};
}