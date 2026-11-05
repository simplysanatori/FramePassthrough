#pragma once

#include "config.hpp"
#include "camera_source.hpp"

#include <cstdint>
#include <string>
#include <optional>

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
    ToggleRgb,
    Rgb,
    Monochrome,
};

class Controller {
public:
    Controller(Settings& settings, Config config, CameraSource* camera_source = nullptr);
    std::string apply(Action action);
    void remember_rgb(bool rgb) { preferred_rgb_ = rgb; }
    std::optional<bool> preferred_rgb() const { return preferred_rgb_; }
    bool reconcile_camera_source();

private:
    Settings& settings_;
    Config config_;
    CameraSource* camera_source_;
    std::optional<bool> preferred_rgb_;
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
