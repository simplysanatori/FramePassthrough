#include "controller.hpp"

#include <exception>
#include <stdexcept>
#include <utility>

namespace fps {
namespace {

constexpr std::int32_t kTranslucent = 3;
constexpr std::int32_t kOpaque = 4;

Value read_typed(Settings& settings, const std::string& section, const std::string& key,
                 const Value& type_hint) {
    auto value = settings.read(section, key, type_hint);
    if (value.index() != type_hint.index()) {
        throw std::runtime_error("Setting " + section + "." + key + " returned an unexpected type");
    }
    return value;
}

} // namespace

Controller::Controller(Settings& settings, Config config, CameraSource* camera_source)
    : settings_(settings), config_(std::move(config)), camera_source_(camera_source) {
    validate_config(config_);
}

void Controller::write_verified(const std::string& section, const std::string& key,
                                 const Value& target) {
    settings_.write(section, key, target);
    if (read_typed(settings_, section, key, target) != target) {
        throw std::runtime_error("Setting " + section + "." + key +
            " did not retain the requested value; SteamVR or the driver may have rejected it");
    }
}

bool Controller::reconcile_camera_source() {
    if (!config_.rgb.enabled || config_.rgb.backend != "steamvr_camera") return false;
    if (!camera_source_) throw std::runtime_error("SteamVR camera source backend is unavailable");
    // XRService owns physical disconnect and streaming fallback. Only restore a
    // chosen source while the RGB module is available; never write on removal.
    if (!preferred_rgb_ || !camera_source_->rgb_available()) return false;
    const bool desired = *preferred_rgb_;
    auto current = camera_source_->read();
    for (auto value : current) if (value > 1) throw std::runtime_error("Invalid camera configuration during source restoration");
    // Wait for SteamVR to initialize/reopen its camera renderer. Never turn it on.
    if (!current[0] || current[2] == static_cast<std::uint8_t>(desired)) return false;
    current[2] = desired ? 1 : 0;
    camera_source_->write(current);
    if (camera_source_->read() != current) throw std::runtime_error("Camera source restoration was rejected");
    return true;
}

std::string Controller::apply(Action action) {
    switch (action) {
    case Action::ToggleStyle:
    case Action::Translucent:
    case Action::Opaque: {
        const auto current = std::get<std::int32_t>(
            read_typed(settings_, "camera", "roomViewStyle", Value{kTranslucent}));
        const auto target = action == Action::Opaque ? kOpaque
            : action == Action::Translucent ? kTranslucent
            : current == kTranslucent ? kOpaque : kTranslucent;
        write_verified("camera", "roomViewStyle", Value{target});
        return target == kOpaque ? "Passthrough style set to opaque"
                                 : "Passthrough style set to translucent";
    }
    case Action::ToggleRgb:
    case Action::Rgb:
    case Action::Monochrome: {
        const auto& rgb = config_.rgb;
        if (!rgb.enabled) {
            throw std::runtime_error("RGB switching is disabled: no verified Arcturus RGB camera selection backend is enabled");
        }
        if (rgb.backend == "steamvr_camera") {
            if (!camera_source_) {
                throw std::runtime_error("SteamVR camera source backend is unavailable");
            }
            if (!camera_source_->rgb_available()) {
                return "Arcturus RGB camera not detected; using built-in monochrome cameras";
            }
            // Read all five bytes for every action so changes made by SteamVR
            // or another application are preserved, including explicit actions.
            auto target = camera_source_->read();
            for (const auto value : target) {
                if (value > 1) {
                    throw std::runtime_error("SteamVR camera configuration is invalid: expected five Boolean values");
                }
            }
            if (target[0] != 1) {
                throw std::runtime_error("SteamVR camera configuration is inactive; refusing to enable cameras");
            }
            // Renderer reopen/reset may temporarily overwrite byte 2. Once a
            // choice exists, toggle that choice rather than the transient flag.
            target[2] = action == Action::ToggleRgb ? static_cast<std::uint8_t>(!preferred_rgb_.value_or(target[2] != 0))
                : action == Action::Rgb ? 1 : 0;
            camera_source_->write(target);
            if (camera_source_->read() != target) {
                throw std::runtime_error("SteamVR camera configuration did not retain the requested value; "
                    "the runtime may have rejected the source change or changed another camera option");
            }
            preferred_rgb_ = target[2] != 0;
            return target[2] ? "Camera source set to RGB" : "Camera source set to monochrome";
        }
        const auto current = read_typed(settings_, rgb.section, rgb.key, rgb.rgb_value);
        Value target;
        if (action == Action::ToggleRgb) {
            if (current == rgb.rgb_value) {
                target = rgb.monochrome_value;
            } else if (current == rgb.monochrome_value) {
                target = rgb.rgb_value;
            } else {
                throw std::runtime_error("Setting " + rgb.section + "." + rgb.key +
                    " is neither the configured RGB nor monochrome value; refusing to guess");
            }
        } else {
            target = action == Action::Rgb ? rgb.rgb_value : rgb.monochrome_value;
        }
        write_verified(rgb.section, rgb.key, target);
        return target == rgb.rgb_value ? "Camera source set to RGB" : "Camera source set to monochrome";
    }
    }
    throw std::runtime_error("Unknown control action");
}

std::string smoke_test(Settings& settings, const Config& config) {
    Controller controls(settings, config);
    // Do not attempt any writes unless the original setting was read correctly.
    const auto original = read_typed(settings, "camera", "roomViewStyle", Value{std::int32_t{0}});
    const auto original_text = std::to_string(std::get<std::int32_t>(original));
    const auto describe = [](const std::exception_ptr& failure) -> std::string {
        try { std::rethrow_exception(failure); }
        catch (const std::exception& error) { return error.what(); }
        catch (...) { return "unknown exception"; }
    };

    std::exception_ptr test_failure;
    try {
        controls.apply(Action::Translucent);
        controls.apply(Action::Opaque);
        controls.apply(Action::ToggleStyle);
    } catch (...) {
        test_failure = std::current_exception();
    }

    // Restoration is explicit so its errors can take precedence over a test
    // failure without throwing from a destructor during exception unwinding.
    try {
        settings.write("camera", "roomViewStyle", original);
        if (read_typed(settings, "camera", "roomViewStyle", original) != original) {
            throw std::runtime_error("restored value did not retain the original setting");
        }
    } catch (...) {
        const auto restore_failure = std::current_exception();
        auto message = "Self-test restoration FAILED. Manual recovery required: restore "
            "camera.roomViewStyle=" + original_text + " in SteamVR settings. Restore error: " +
            describe(restore_failure);
        if (test_failure) message += ". Original test error: " + describe(test_failure);
        throw std::runtime_error(message);
    }

    if (test_failure) {
        throw std::runtime_error("Self-test failed; restored camera.roomViewStyle=" +
            original_text + ": " + describe(test_failure));
    }
    return "Passthrough settings self-test passed: translucent, opaque, toggle; restored "
        "camera.roomViewStyle=" + original_text +
        ". Visual output and physical controller bindings were not tested.";
}

PressGate::PressGate(std::uint32_t debounce_ms) : debounce_ms_(debounce_ms) {}

bool PressGate::update(bool active, bool down, bool changed, std::uint64_t monotonic_ms) {
    if (!active) {
        reset();
        return false;
    }
    if (!down) {
        armed_ = true;
        return false;
    }
    if (!armed_) {
        return false;
    }
    // Consume this press even if it is rejected, so a hold can never fire later.
    armed_ = false;
    if (!changed) {
        return false;
    }
    if (fired_ && (monotonic_ms < last_fire_ms_ ||
        monotonic_ms - last_fire_ms_ < debounce_ms_)) {
        return false;
    }
    fired_ = true;
    last_fire_ms_ = monotonic_ms;
    return true;
}

void PressGate::reset() {
    // SteamVR can activate a gesture action on its first rising edge. Requiring
    // an active release sample here would discard that first double/long press.
    // update still requires bChanged and consumes a down sample until release.
    armed_ = true;
}

} // namespace fps
