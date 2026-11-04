#pragma once

#include "config.hpp"

#include <cstdint>
#include <string>

namespace fps {

class Settings {
public:
    virtual ~Settings() = default;
    virtual Value read(const std::string& section, const std::string& key,
                       const Value& type_hint) = 0;
    virtual void write(const std::string& section, const std::string& key,
                       const Value& value) = 0;
};

enum class Action {
    ToggleStyle,
    Translucent,
    Opaque,
};

class Controller {
public:
    Controller(Settings& settings, Config config);
    std::string apply(Action action);

private:
    Settings& settings_;
    Config config_;
    void write_verified(const std::string& section, const std::string& key, const Value& target);
};

// Exercise only Room View style settings, restoring the exact original value.
// Throws on a test or restoration failure; never changes camera enable/source.
std::string smoke_test(Settings& settings, const Config& config);

class PressGate {
public:
    explicit PressGate(std::uint32_t debounce_ms = 250);
    bool update(bool active, bool down, bool changed, std::uint64_t monotonic_ms);
    void reset();

private:
    std::uint32_t debounce_ms_;
    bool armed_ = true;
    bool fired_ = false;
    std::uint64_t last_fire_ms_ = 0;
};

} // namespace fps
