#include "config.hpp"

#include <json.hpp>

#include <algorithm>
#include <fstream>
#include <initializer_list>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace fps {
namespace {

using Json = nlohmann::json;

[[noreturn]] void invalid(const std::string& detail) {
    throw std::runtime_error("Invalid configuration: " + detail);
}

void check_keys(const Json& object, std::initializer_list<const char*> allowed,
                const std::string& location) {
    if (!object.is_object()) {
        invalid(location + " must be an object");
    }
    for (const auto& item : object.items()) {
        if (std::none_of(allowed.begin(), allowed.end(), [&](const char* key) {
                return item.key() == key;
            })) {
            invalid("unknown key " + location + "." + item.key());
        }
    }
}

std::int64_t integer(const Json& value, const std::string& location,
                     std::int64_t minimum, std::int64_t maximum) {
    if (!value.is_number_integer()) {
        invalid(location + " must be an integer");
    }
    // Check unsigned JSON integers before conversion to avoid overflow.
    if (value.is_number_unsigned()) {
        const auto number = value.get<std::uint64_t>();
        if (number > static_cast<std::uint64_t>(maximum)) {
            invalid(location + " is out of range");
        }
    }
    const auto number = value.get<std::int64_t>();
    if (number < minimum || number > maximum) {
        invalid(location + " is out of range");
    }
    return number;
}

std::string string_value(const Json& value, const std::string& location) {
    if (!value.is_string()) {
        invalid(location + " must be a string");
    }
    auto result = value.get<std::string>();
    if (result.find('\0') != std::string::npos) {
        invalid(location + " must not contain NUL characters");
    }
    return result;
}

Value setting_value(const Json& value, const std::string& location) {
    if (value.is_boolean()) {
        return value.get<bool>();
    }
    if (value.is_number_integer()) {
        return static_cast<std::int32_t>(integer(value, location,
            std::numeric_limits<std::int32_t>::min(),
            std::numeric_limits<std::int32_t>::max()));
    }
    if (value.is_string()) {
        return string_value(value, location);
    }
    invalid(location + " must be a boolean, 32-bit integer, or string");
}

bool empty_name(const std::string& name) {
    return name.empty() || name.find_first_not_of(" \t\r\n") == std::string::npos;
}

void check_no_nul(const std::string& value, const std::string& location) {
    if (value.find('\0') != std::string::npos) {
        invalid(location + " must not contain NUL characters");
    }
}

} // namespace

void validate_config(const Config& config) {
    const auto& signal = config.passthrough_signal;
    if (signal != "auto" && signal != "setting" && signal != "events" && signal != "steamvr_log" && signal != "off") {
        invalid("passthrough_signal must be auto, setting, events, steamvr_log or off");
    }
    if (config.poll_interval_ms < 10 || config.poll_interval_ms > 1000) {
        invalid("poll_interval_ms must be between 10 and 1000");
    }
    if (config.debounce_ms > 5000) {
        invalid("debounce_ms must be between 0 and 5000");
    }
    const auto& rgb = config.rgb;
    if (rgb.backend != "settings" && rgb.backend != "steamvr_camera") {
        invalid("rgb.backend must be settings or steamvr_camera");
    }
    check_no_nul(rgb.section, "rgb.section");
    check_no_nul(rgb.key, "rgb.key");
    if (rgb.backend == "steamvr_camera" && (!rgb.section.empty() || !rgb.key.empty())) {
        invalid("rgb.section and rgb.key must be empty for the steamvr_camera backend");
    }
    if (rgb.rgb_value.index() != rgb.monochrome_value.index()) {
        invalid("rgb.rgb_value and rgb.monochrome_value must have the same type");
    }
    if (rgb.rgb_value == rgb.monochrome_value) {
        invalid("rgb.rgb_value and rgb.monochrome_value must be distinct");
    }
    if (const auto* value = std::get_if<std::string>(&rgb.rgb_value)) {
        check_no_nul(*value, "rgb.rgb_value");
        check_no_nul(std::get<std::string>(rgb.monochrome_value), "rgb.monochrome_value");
    }
    if (rgb.enabled && rgb.backend == "settings" &&
        (empty_name(rgb.section) || empty_name(rgb.key))) {
        invalid("enabled RGB control requires rgb.section and rgb.key");
    }
    if (rgb.section == "camera" && rgb.key == "enableCamera") {
        invalid("camera.enableCamera requires a restart and does not select RGB or monochrome cameras");
    }
    if (rgb.section == "camera" && (rgb.key == "roomViewStyle" || rgb.key == "roomView")) {
        invalid("RGB control must not overwrite camera." + rgb.key);
    }
}

Config parse_config(const std::string& json_text) {
    Json document;
    try {
        // Reject duplicate keys instead of silently accepting the last value.
        std::vector<std::vector<std::string>> keys;
        document = Json::parse(json_text, [&](int, Json::parse_event_t event, Json& parsed) {
            if (event == Json::parse_event_t::object_start) {
                keys.emplace_back();
            } else if (event == Json::parse_event_t::object_end) {
                keys.pop_back();
            } else if (event == Json::parse_event_t::key) {
                auto key = parsed.get<std::string>();
                auto& object_keys = keys.back();
                if (std::find(object_keys.begin(), object_keys.end(), key) != object_keys.end()) {
                    invalid("duplicate key " + key);
                }
                object_keys.push_back(std::move(key));
            }
            return true;
        });
    } catch (const Json::exception& error) {
        invalid(error.what());
    }
    check_keys(document, {"poll_interval_ms", "debounce_ms", "rgb", "passthrough_signal",
        "disable_while_game_running"}, "config");
    Config config;
    if (document.contains("poll_interval_ms")) {
        config.poll_interval_ms = static_cast<std::uint32_t>(integer(
            document.at("poll_interval_ms"), "poll_interval_ms", 10, 1000));
    }
    if (document.contains("debounce_ms")) {
        config.debounce_ms = static_cast<std::uint32_t>(integer(
            document.at("debounce_ms"), "debounce_ms", 0, 5000));
    }
    if (document.contains("disable_while_game_running")) {
        if (!document.at("disable_while_game_running").is_boolean()) {
            invalid("disable_while_game_running must be true or false");
        }
        config.disable_while_game_running = document.at("disable_while_game_running").get<bool>();
    }
    if (document.contains("passthrough_signal")) {
        config.passthrough_signal = string_value(document.at("passthrough_signal"), "passthrough_signal");
    }
    if (document.contains("rgb")) {
        const auto& rgb = document.at("rgb");
        check_keys(rgb, {"enabled", "backend", "section", "key", "rgb_value", "monochrome_value"}, "rgb");
        if (rgb.contains("enabled")) {
            if (!rgb.at("enabled").is_boolean()) {
                invalid("rgb.enabled must be a boolean");
            }
            config.rgb.enabled = rgb.at("enabled").get<bool>();
        }
        if (rgb.contains("backend")) {
            config.rgb.backend = string_value(rgb.at("backend"), "rgb.backend");
        }
        if (rgb.contains("section")) {
            config.rgb.section = string_value(rgb.at("section"), "rgb.section");
        }
        if (rgb.contains("key")) {
            config.rgb.key = string_value(rgb.at("key"), "rgb.key");
        }
        if (rgb.contains("rgb_value")) {
            config.rgb.rgb_value = setting_value(rgb.at("rgb_value"), "rgb.rgb_value");
        }
        if (rgb.contains("monochrome_value")) {
            config.rgb.monochrome_value = setting_value(rgb.at("monochrome_value"), "rgb.monochrome_value");
        }
    }
    validate_config(config);
    return config;
}

std::string set_config_option(const std::string& json_text, const std::string& key, bool value) {
    (void)parse_config(json_text); // refuse to rewrite a file that is already invalid
    auto document = nlohmann::ordered_json::parse(json_text);
    document[key] = value;
    auto text = document.dump(2) + "\n";
    (void)parse_config(text);
    return text;
}

Config load_config(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        throw std::runtime_error("Cannot open configuration: " + path.string());
    }
    std::ostringstream contents;
    contents << stream.rdbuf();
    if (stream.bad()) {
        throw std::runtime_error("Cannot read configuration: " + path.string());
    }
    return parse_config(contents.str());
}

} // namespace fps
