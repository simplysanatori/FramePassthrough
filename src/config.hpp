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
    // Which signal decides that passthrough is visible: auto, setting,
    // events, steamvr_log, or off (never gate on passthrough).
    std::string passthrough_signal = "auto";
    // Disable the gestures while a game (any scene application other than a
    // SteamVR home) is running, even when passthrough is visible. On by
    // default: the shortcuts are for the home view; false lets them act in
    // games too.
    bool disable_while_game_running = true;
};

// Returns json_text with one top-level option set, keeping the other keys
// and their order. The result is validated with parse_config.
std::string set_config_option(const std::string& json_text, const std::string& key, bool value);

Config parse_config(const std::string& json_text);
Config load_config(const std::filesystem::path& path);
void validate_config(const Config& config);

} // namespace fps
