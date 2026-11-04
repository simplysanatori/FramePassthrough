#include "config.hpp"

#include <json.hpp>

#include <algorithm>
#include <fstream>
#include <initializer_list>
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

} // namespace

void validate_config(const Config& config) {
    if (config.poll_interval_ms < 10 || config.poll_interval_ms > 1000) {
        invalid("poll_interval_ms must be between 10 and 1000");
    }
    if (config.debounce_ms > 5000) {
        invalid("debounce_ms must be between 0 and 5000");
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
    check_keys(document, {"poll_interval_ms", "debounce_ms"}, "config");
    Config config;
    if (document.contains("poll_interval_ms")) {
        config.poll_interval_ms = static_cast<std::uint32_t>(integer(
            document.at("poll_interval_ms"), "poll_interval_ms", 10, 1000));
    }
    if (document.contains("debounce_ms")) {
        config.debounce_ms = static_cast<std::uint32_t>(integer(
            document.at("debounce_ms"), "debounce_ms", 0, 5000));
    }
    validate_config(config);
    return config;
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
