#pragma once

#include "gate.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <optional>
#include <string>

#include <sys/types.h>

namespace fps {

// Which signal decides whether passthrough is visible.
enum class PassthroughSignal {
    Auto,       // Setting while no scene app runs; newest of Events / SteamVrLog
                // while one does
    Setting,    // camera.enableConstructRoomView only
    Events,     // OpenVR VREvent_RoomViewShown / VREvent_RoomViewHidden only
    SteamVrLog, // XRService room view rendering mode, as logged by vrserver
    Off,        // do not gate on passthrough at all (upstream behaviour)
};

std::optional<PassthroughSignal> parse_passthrough_signal(const std::string& text);
const char* to_string(PassthroughSignal signal);

// XRService reports its session settings to vrserver, which logs them as
//   "currentRoomViewRenderingMode": 6
// 0 means Room View is not rendered; any other value means it is.
std::optional<int> parse_room_view_rendering_mode(const std::string& line);

// Follows a growing text file, reopening it after rotation. The first poll
// reads the existing contents so the newest logged state is known at once.
class LogFollower {
public:
    explicit LogFollower(std::filesystem::path path);
    void poll(const std::function<void(const std::string&)>& on_line);
    const std::filesystem::path& path() const { return path_; }

private:
    std::filesystem::path path_;
    std::ifstream stream_;
    ino_t inode_ = 0;
    std::streamoff offset_ = 0;
};

// Combines the passthrough signals into one Visibility.
//
// Proven on SteamVR 2.19.1 / Steam Frame (probe session 2026-11-05, home
// view, no scene app): every passthrough open and close flips the setting
// camera.enableConstructRoomView (announced by VREvent_CameraSettingsHaveChanged),
// XRService's logged currentRoomViewRenderingMode (6 / 0) and the
// compositor's RoomViewFullyOccludingApp flag together, and nothing else
// (dashboard open / close, standby) flips them. The setting changes first
// and is public API, so it decides in the home view. Room View over a
// running scene app is reported by VREvent_RoomViewShown / Hidden ("for
// scene apps only") and by the XRService mode, so those decide then.
class PassthroughState {
public:
    PassthroughState(PassthroughSignal signal, std::filesystem::path vrserver_log);

    void set_signal(PassthroughSignal signal) { signal_ = signal; }
    PassthroughSignal signal() const { return signal_; }

    // Feed VREvent_RoomViewShown (true) / VREvent_RoomViewHidden (false).
    void on_room_view_event(bool shown);
    // Current value of camera.enableConstructRoomView.
    void on_construct_setting(bool enabled);
    // Whether SteamVR has a scene application (any process, game or home).
    void set_scene_app_running(bool running) { scene_running_ = running; }
    // Read new SteamVR log lines. Cheap when nothing changed (one stat).
    void poll();

    Visibility visibility() const;
    // Human readable source of the current value, for logs and runtime.json.
    std::string detail() const;

private:
    struct Observation {
        Visibility value = Visibility::Unknown;
        std::uint64_t sequence = 0;
    };
    PassthroughSignal signal_;
    LogFollower log_;
    Observation from_events_;
    Observation from_log_;
    Observation from_setting_;
    bool scene_running_ = false;
    int last_mode_ = -1;
    std::uint64_t sequence_ = 0;
};

} // namespace fps
