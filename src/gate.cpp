#include "gate.hpp"

#include <array>

namespace fps {

const char* to_string(Visibility visibility) {
    switch (visibility) {
    case Visibility::Unknown: return "unknown";
    case Visibility::Hidden: return "hidden";
    case Visibility::Visible: return "visible";
    }
    return "unknown";
}

const char* to_string(GateReason reason) {
    switch (reason) {
    case GateReason::PassthroughVisible: return "passthrough visible";
    case GateReason::PassthroughUnknown: return "passthrough state unknown, failing open";
    case GateReason::PassthroughHidden: return "passthrough hidden";
    case GateReason::GameRunning: return "a game is running and disable_while_game_running is on";
    }
    return "unknown";
}

InputGate::InputGate(GateTimings timings) : timings_(timings) {}

bool InputGate::update(const GateInputs& inputs, std::uint64_t now_ms) {
    bool open = false;
    GateReason reason = GateReason::PassthroughHidden;
    if (inputs.disable_while_game_running && inputs.game_running) {
        reason = GateReason::GameRunning;
    } else if (inputs.passthrough == Visibility::Visible) {
        open = true;
        reason = GateReason::PassthroughVisible;
    } else if (inputs.passthrough == Visibility::Unknown) {
        open = true;
        reason = GateReason::PassthroughUnknown;
    }
    const bool changed = !initialized_ || open != open_ || reason != reason_;
    // Visible <-> unknown keeps the gate open; only a closed -> open edge
    // restarts the guard period.
    if (open && (!initialized_ || !open_)) opened_at_ms_ = now_ms;
    initialized_ = true;
    open_ = open;
    reason_ = reason;
    return changed;
}

bool InputGate::accept(GestureKind kind, std::uint64_t now_ms) const {
    if (!open_) return false;
    const std::uint32_t guard = kind == GestureKind::Hold ? timings_.hold_guard_ms
                                                          : timings_.double_press_guard_ms;
    if (now_ms < opened_at_ms_) return false;
    return now_ms - opened_at_ms_ >= guard;
}

bool is_game_scene_app(std::uint32_t scene_pid, const std::string& app_key) {
    if (scene_pid == 0) return false;
    // SteamVR Home and the stock environments are homes, not games. On Steam
    // Frame no scene app runs in the home view at all (scene pid 0).
    static const std::array<const char*, 3> homes = {
        "openvr.tool.steamvr_environments",
        "openvr.tool.steamvr_environments_tools",
        "steam.app.250820", // SteamVR Home
    };
    for (const auto* home : homes)
        if (app_key == home) return false;
    // SteamVR components and the Frame's shell processes are never games.
    static const std::array<const char*, 6> system_keys = {
        "openvr.component.",
        "system.generated.gamescope",
        "system.generated.steamwebhelper",
        "system.generated.xrservice",
        "system.systemui",
        "steam.client",
    };
    for (const auto* prefix : system_keys)
        if (app_key.rfind(prefix, 0) == 0) return false;
    return true;
}

} // namespace fps
