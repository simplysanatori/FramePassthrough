#pragma once

#include <cstdint>
#include <string>

namespace fps {

// What we currently believe about passthrough / Room View visibility.
enum class Visibility {
    Unknown, // no signal yet; the gate fails open so the app never goes dead
    Hidden,
    Visible,
};

const char* to_string(Visibility visibility);

// Why the gate is open or closed. Logged on every transition.
enum class GateReason {
    PassthroughVisible,
    PassthroughUnknown,
    PassthroughHidden,
    GameRunning,
};

const char* to_string(GateReason reason);

// How long a gesture needs to complete. Hold gestures need a longer guard
// after the gate opens than double presses do.
enum class GestureKind {
    DoublePress,
    Hold,
};

struct GateInputs {
    Visibility passthrough = Visibility::Unknown;
    bool game_running = false;
    bool disable_while_game_running = false;
};

struct GateTimings {
    // A double press completes at most double_press_delay after its first
    // press; reject edges for this long after opening. Defaults to debounce.
    std::uint32_t double_press_guard_ms = 250;
    // A hold fires long_press_delay (1.0 s) after the press began. Any hold
    // firing sooner than this after the gate opened started while it was
    // closed.
    std::uint32_t hold_guard_ms = 1050;
};

// Pure state machine deciding whether the shortcut gestures may act.
// No SteamVR calls; driven by update() and asked through accept().
class InputGate {
public:
    explicit InputGate(GateTimings timings = {});

    // Returns true when the open/closed state or the reason changed.
    bool update(const GateInputs& inputs, std::uint64_t now_ms);

    bool open() const { return open_; }
    GateReason reason() const { return reason_; }
    std::uint64_t opened_at_ms() const { return opened_at_ms_; }

    // A press that a PressGate accepted. Returns false when the gate is
    // closed or the press may have started before the gate opened.
    bool accept(GestureKind kind, std::uint64_t now_ms) const;

    void set_timings(GateTimings timings) { timings_ = timings; }

private:
    GateTimings timings_;
    bool initialized_ = false;
    bool open_ = false;
    GateReason reason_ = GateReason::PassthroughHidden;
    std::uint64_t opened_at_ms_ = 0;
};

// Classification of SteamVR's current scene application.
// Returns true when the scene app counts as a game: any scene process other
// than SteamVR's own home environments or system components.
bool is_game_scene_app(std::uint32_t scene_pid, const std::string& app_key);

} // namespace fps
