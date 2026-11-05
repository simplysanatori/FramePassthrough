#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <variant>

namespace fps {

using Value = std::variant<bool, std::int32_t, std::string>;

struct RgbConfig {
    bool enabled = false;
    std::string backend = "settings";
    std::string section;
    std::string key;
    Value rgb_value = true;
    Value monochrome_value = false;
};

struct Config {
    std::uint32_t poll_interval_ms = 20;
    std::uint32_t debounce_ms = 250;
    RgbConfig rgb;
};

Config parse_config(const std::string& json_text);
Config load_config(const std::filesystem::path& path);
void validate_config(const Config& config);

} // namespace fps
