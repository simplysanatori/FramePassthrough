#include "passthrough_state.hpp"

#include <cctype>
#include <cstdlib>
#include <utility>

#include <sys/stat.h>

namespace fps {

std::optional<PassthroughSignal> parse_passthrough_signal(const std::string& text) {
    if (text == "auto") return PassthroughSignal::Auto;
    if (text == "events") return PassthroughSignal::Events;
    if (text == "setting") return PassthroughSignal::Setting;
    if (text == "steamvr_log") return PassthroughSignal::SteamVrLog;
    if (text == "off") return PassthroughSignal::Off;
    return std::nullopt;
}

const char* to_string(PassthroughSignal signal) {
    switch (signal) {
    case PassthroughSignal::Auto: return "auto";
    case PassthroughSignal::Events: return "events";
    case PassthroughSignal::Setting: return "setting";
    case PassthroughSignal::SteamVrLog: return "steamvr_log";
    case PassthroughSignal::Off: return "off";
    }
    return "auto";
}

std::optional<int> parse_room_view_rendering_mode(const std::string& line) {
    static const std::string key = "\"currentRoomViewRenderingMode\"";
    const auto at = line.find(key);
    if (at == std::string::npos) return std::nullopt;
    auto pos = at + key.size();
    while (pos < line.size() && std::isspace(static_cast<unsigned char>(line[pos]))) ++pos;
    if (pos >= line.size() || line[pos] != ':') return std::nullopt;
    ++pos;
    while (pos < line.size() && std::isspace(static_cast<unsigned char>(line[pos]))) ++pos;
    const auto start = pos;
    if (pos < line.size() && line[pos] == '-') ++pos;
    while (pos < line.size() && std::isdigit(static_cast<unsigned char>(line[pos]))) ++pos;
    if (pos == start || (pos == start + 1 && line[start] == '-')) return std::nullopt;
    if (pos - start > 9) return std::nullopt;
    return std::atoi(line.substr(start, pos - start).c_str());
}

LogFollower::LogFollower(std::filesystem::path path) : path_(std::move(path)) {}

void LogFollower::poll(const std::function<void(const std::string&)>& on_line) {
    struct stat info {};
    if (::stat(path_.c_str(), &info) != 0) {
        stream_.close();
        return;
    }
    const bool rotated = info.st_ino != inode_ || static_cast<std::streamoff>(info.st_size) < offset_;
    if (!stream_.is_open() || rotated) {
        stream_.close();
        stream_.clear();
        stream_.open(path_, std::ios::binary);
        if (!stream_) return;
        inode_ = info.st_ino;
        offset_ = 0;
    }
    if (static_cast<std::streamoff>(info.st_size) == offset_) return;
    stream_.clear();
    stream_.seekg(offset_);
    std::string line;
    while (std::getline(stream_, line)) {
        // An unterminated last line is still being written; read it later.
        if (stream_.eof()) break;
        offset_ = stream_.tellg();
        on_line(line);
    }
    stream_.clear();
}

PassthroughState::PassthroughState(PassthroughSignal signal, std::filesystem::path vrserver_log)
    : signal_(signal), log_(std::move(vrserver_log)) {}

void PassthroughState::on_room_view_event(bool shown) {
    from_events_ = {shown ? Visibility::Visible : Visibility::Hidden, ++sequence_};
}

void PassthroughState::on_construct_setting(bool enabled) {
    const auto value = enabled ? Visibility::Visible : Visibility::Hidden;
    if (from_setting_.value != value) from_setting_ = {value, ++sequence_};
}

void PassthroughState::poll() {
    if (signal_ != PassthroughSignal::Auto && signal_ != PassthroughSignal::SteamVrLog) return;
    log_.poll([this](const std::string& line) {
        const auto mode = parse_room_view_rendering_mode(line);
        if (!mode) return;
        last_mode_ = *mode;
        from_log_ = {*mode != 0 ? Visibility::Visible : Visibility::Hidden, ++sequence_};
    });
}

Visibility PassthroughState::visibility() const {
    switch (signal_) {
    case PassthroughSignal::Off: return Visibility::Visible;
    case PassthroughSignal::Events: return from_events_.value;
    case PassthroughSignal::Setting: return from_setting_.value;
    case PassthroughSignal::SteamVrLog: return from_log_.value;
    case PassthroughSignal::Auto:
        if (!scene_running_) return from_setting_.value;
        if (from_events_.value == Visibility::Unknown) return from_log_.value;
        if (from_log_.value == Visibility::Unknown) return from_events_.value;
        return from_events_.sequence > from_log_.sequence ? from_events_.value : from_log_.value;
    }
    return Visibility::Unknown;
}

std::string PassthroughState::detail() const {
    std::string text = std::string("signal=") + to_string(signal_);
    text += std::string(", scene_app=") + (scene_running_ ? "yes" : "no");
    text += ", construct_setting=" + std::string(to_string(from_setting_.value));
    text += ", event=" + std::string(to_string(from_events_.value));
    text += ", xrservice_mode=" + (last_mode_ < 0 ? std::string("none") : std::to_string(last_mode_));
    return text;
}

} // namespace fps
