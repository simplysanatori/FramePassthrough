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
    Auto,       // newest observation from either source below
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
class PassthroughState {
public:
    PassthroughState(PassthroughSignal signal, std::filesystem::path vrserver_log);

    void set_signal(PassthroughSignal signal) { signal_ = signal; }
    PassthroughSignal signal() const { return signal_; }

    // Feed VREvent_RoomViewShown (true) / VREvent_RoomViewHidden (false).
    void on_room_view_event(bool shown);
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
    int last_mode_ = -1;
    std::uint64_t sequence_ = 0;
};

} // namespace fps
