// Read-only SteamVR state probe for FramePassthrough development.
//
// Connects as VRApplication_Background, never registers input, an app key or
// a manifest, and never writes SteamVR settings. It logs every VREvent plus
// a set of candidate "passthrough visible" and "scene application" signals,
// and mirrors relevant lines from SteamVR's own logs so they share one
// timeline. Reconnects if SteamVR restarts.
#include <openvr.h>

#include <array>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <sys/stat.h>
#include <unistd.h>

namespace fs = std::filesystem;

namespace {

std::atomic<bool> stopping{false};
void on_signal(int) { stopping = true; }

std::ofstream out_file;

std::string timestamp() {
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto t = system_clock::to_time_t(now);
    std::tm tm{};
    localtime_r(&t, &tm);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    const auto us = duration_cast<microseconds>(now.time_since_epoch()).count() % 1000000;
    char full[80];
    std::snprintf(full, sizeof(full), "%s.%06lld", buf, static_cast<long long>(us));
    return full;
}

void emit(const std::string& kind, const std::string& text) {
    const auto line = timestamp() + " " + kind + " " + text;
    if (out_file.is_open()) out_file << line << '\n' << std::flush;
    else std::cout << line << std::endl;
}

std::string exe_name(uint32_t pid) {
    if (!pid) return "-";
    std::error_code error;
    const auto target = fs::read_symlink(fs::path("/proc") / std::to_string(pid) / "exe", error);
    return error ? "?" : target.filename().string();
}

std::string app_key_for(uint32_t pid) {
    if (!pid || !vr::VRApplications()) return "-";
    std::array<char, vr::k_unMaxApplicationKeyLength> key{};
    const auto error = vr::VRApplications()->GetApplicationKeyByProcessId(pid, key.data(), static_cast<uint32_t>(key.size()));
    return error == vr::VRApplicationError_None ? std::string(key.data()) : std::string("err") + std::to_string(error);
}

std::string describe_pid(uint32_t pid) {
    return std::to_string(pid) + "/" + exe_name(pid) + "/" + app_key_for(pid);
}

std::string setting_int(const char* section, const char* key) {
    vr::EVRSettingsError error = vr::VRSettingsError_None;
    const auto value = vr::VRSettings()->GetInt32(section, key, &error);
    return error == vr::VRSettingsError_None ? std::to_string(value) : std::string("err") + std::to_string(error);
}

std::string setting_bool(const char* section, const char* key) {
    vr::EVRSettingsError error = vr::VRSettingsError_None;
    const auto value = vr::VRSettings()->GetBool(section, key, &error);
    return error == vr::VRSettingsError_None ? (value ? "1" : "0") : std::string("err") + std::to_string(error);
}

// Same private getter the shortcuts app has always used (slot 9 of
// IVRCameraPassthroughInternal_001). Read only; the setter is never touched.
std::string camera_internal() {
    vr::EVRInitError error = vr::VRInitError_None;
    void* instance = vr::VR_GetGenericInterface("IVRCameraPassthroughInternal_001", &error);
    if (!instance || error != vr::VRInitError_None) return "unavailable";
    auto** methods = *reinterpret_cast<void***>(instance);
    if (!methods || !methods[9]) return "no-getter";
    std::array<std::uint8_t, 5> config;
    config.fill(0xff);
    using GetConfig = bool (*)(void*, std::uint8_t*);
    const bool ret = reinterpret_cast<GetConfig>(methods[9])(instance, config.data());
    std::ostringstream text;
    text << "ret=" << ret << " bytes=";
    for (auto b : config) text << static_cast<int>(b);
    return text.str();
}

using State = std::map<std::string, std::string>;

State read_state() {
    State s;
    s["set.camera.roomView"] = setting_int("camera", "roomView");
    s["set.camera.roomViewStyle"] = setting_int("camera", "roomViewStyle");
    s["set.camera.enableCamera"] = setting_bool("camera", "enableCamera");
    s["set.camera.enableConstructRoomView"] = setting_bool("camera", "enableConstructRoomView");
    s["set.steamvr.globalActionSetPriority"] = setting_bool("steamvr", "globalActionSetPriority");
    s["cam.internal"] = camera_internal();
    if (auto* overlay = vr::VROverlay()) {
        s["ovl.dashboardVisible"] = overlay->IsDashboardVisible() ? "1" : "0";
        s["ovl.primaryDashboardDevice"] = std::to_string(static_cast<int>(overlay->GetPrimaryDashboardDevice()));
    }
    if (auto* apps = vr::VRApplications()) {
        s["app.sceneState"] = apps->GetSceneApplicationStateNameFromEnum(apps->GetSceneApplicationState());
        s["app.sceneProcess"] = describe_pid(apps->GetCurrentSceneProcessId());
        std::array<char, vr::k_unMaxApplicationKeyLength> key{};
        const auto error = apps->GetStartingApplication(key.data(), static_cast<uint32_t>(key.size()));
        s["app.starting"] = error == vr::VRApplicationError_None ? std::string(key.data()) : "-";
    }
    if (auto* compositor = vr::VRCompositor()) {
        s["cmp.sceneFocus"] = describe_pid(compositor->GetCurrentSceneFocusProcess());
        s["cmp.lastFrameRenderer"] = describe_pid(compositor->GetLastFrameRenderer());
        s["cmp.isFullscreen"] = compositor->IsFullscreen() ? "1" : "0";
        s["cmp.canRenderScene"] = compositor->CanRenderScene() ? "1" : "0";
        s["cmp.sceneFocusLoading"] = compositor->IsCurrentSceneFocusAppLoading() ? "1" : "0";
    } else {
        s["cmp"] = "unavailable";
    }
    if (auto* system = vr::VRSystem()) {
        s["sys.inputAvailable"] = system->IsInputAvailable() ? "1" : "0";
        s["sys.drawingControllers"] = system->IsSteamVRDrawingControllers() ? "1" : "0";
        s["sys.shouldPause"] = system->ShouldApplicationPause() ? "1" : "0";
        s["sys.reduceRendering"] = system->ShouldApplicationReduceRenderingWork() ? "1" : "0";
        s["sys.hmdActivity"] = std::to_string(static_cast<int>(system->GetTrackedDeviceActivityLevel(vr::k_unTrackedDeviceIndex_Hmd)));
    }
    if (auto* camera = vr::VRTrackedCamera()) {
        bool has = false;
        const auto error = camera->HasCamera(vr::k_unTrackedDeviceIndex_Hmd, &has);
        s["trk.hasCamera"] = error == vr::VRTrackedCameraError_None ? (has ? "1" : "0") : std::string("err") + std::to_string(error);
    }
    return s;
}

std::string hex(const void* data, size_t size) {
    static const char* digits = "0123456789abcdef";
    std::string text;
    const auto* bytes = static_cast<const unsigned char*>(data);
    for (size_t i = 0; i < size; ++i) {
        text += digits[bytes[i] >> 4];
        text += digits[bytes[i] & 15];
    }
    return text;
}

std::string describe_event(const vr::VREvent_t& event) {
    std::ostringstream text;
    const char* name = vr::VRSystem()->GetEventTypeNameFromEnum(static_cast<vr::EVREventType>(event.eventType));
    text << (name ? name : "?") << "(" << event.eventType << ") dev=" << event.trackedDeviceIndex
         << " age=" << event.eventAgeSeconds;
    switch (event.eventType) {
    case vr::VREvent_SceneApplicationChanged:
    case vr::VREvent_InputFocusChanged:
    case vr::VREvent_SceneApplicationStateChanged:
    case vr::VREvent_ProcessConnected:
    case vr::VREvent_ProcessDisconnected:
    case vr::VREvent_ProcessQuit:
    case vr::VREvent_Quit:
    case vr::VREvent_DashboardActivated:
    case vr::VREvent_DashboardDeactivated:
        text << " pid=" << describe_pid(event.data.process.pid) << " oldPid=" << describe_pid(event.data.process.oldPid)
             << " forced=" << event.data.process.bForced;
        break;
    case vr::VREvent_Input_BindingLoadFailed:
    case vr::VREvent_Input_BindingLoadSuccessful:
        text << " appContainer=" << event.data.inputBinding.ulAppContainer
             << " pathMessage=" << event.data.inputBinding.pathMessage
             << " pathUrl=" << event.data.inputBinding.pathUrl
             << " pathControllerType=" << event.data.inputBinding.pathControllerType;
        break;
    case vr::VREvent_PropertyChanged:
        text << " container=" << event.data.property.container << " prop=" << event.data.property.prop;
        break;
    default:
        break;
    }
    text << " data=" << hex(&event.data, 32);
    return text.str();
}

// Mirrors new lines of a SteamVR log that match one of the needles.
class LogTail {
public:
    LogTail(fs::path path, std::vector<std::string> needles)
        : path_(std::move(path)), needles_(std::move(needles)) {}

    void poll() {
        struct stat info {};
        if (::stat(path_.c_str(), &info) != 0) return;
        if (!stream_.is_open() || info.st_ino != inode_ || static_cast<std::streamoff>(info.st_size) < offset_) {
            stream_.close();
            stream_.clear();
            stream_.open(path_);
            if (!stream_) return;
            inode_ = info.st_ino;
            // On first open skip history; after a rotation read the new file from the start.
            if (!opened_once_) stream_.seekg(0, std::ios::end);
            opened_once_ = true;
            offset_ = stream_.tellg();
        }
        stream_.clear();
        stream_.seekg(offset_);
        std::string line;
        while (std::getline(stream_, line)) {
            if (stream_.eof()) break; // incomplete line, re-read later
            offset_ = stream_.tellg();
            if (want_context_ > 0) {
                --want_context_;
                if (line.find("isCameraViewEnabled") != std::string::npos ||
                    line.find("currentRoomViewRenderingMode") != std::string::npos)
                    emit("LOG", path_.filename().string() + ":   " + line);
                continue;
            }
            for (const auto& needle : needles_) {
                if (line.find(needle) != std::string::npos) {
                    emit("LOG", path_.filename().string() + ": " + line);
                    if (line.find("SessionSettingsChanged") != std::string::npos) want_context_ = 6;
                    break;
                }
            }
        }
        stream_.clear();
    }

private:
    fs::path path_;
    std::vector<std::string> needles_;
    std::ifstream stream_;
    ino_t inode_ = 0;
    std::streamoff offset_ = 0;
    bool opened_once_ = false;
    int want_context_ = 0;
};

} // namespace

int main(int argc, char** argv) {
    std::signal(SIGINT, on_signal);
    std::signal(SIGTERM, on_signal);
    std::signal(SIGHUP, SIG_IGN);
    int interval_ms = 20;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--log" && i + 1 < argc) {
            out_file.open(argv[++i], std::ios::app);
            if (!out_file) { std::cerr << "cannot open log\n"; return 1; }
        } else if (arg == "--interval-ms" && i + 1 < argc) {
            interval_ms = std::max(5, std::atoi(argv[++i]));
        } else {
            std::cerr << "usage: fp-probe [--log PATH] [--interval-ms N]\n";
            return 2;
        }
    }
    const char* home = std::getenv("HOME");
    const fs::path logs = fs::path(home ? home : "/") / ".local/share/Steam/logs";
    std::vector<LogTail> tails;
    tails.emplace_back(logs / "vrserver.txt", std::vector<std::string>{"SessionSettingsChanged", "RoomView", "roomview", "ToggleRoomView"});
    tails.emplace_back(logs / "vrclient_vrcompositor.txt", std::vector<std::string>{"SystemBehaviorFlag", "RoomView", "roomview"});
    tails.emplace_back(logs / "vrcompositor.txt", std::vector<std::string>{"RoomView", "roomview", "Passthrough", "passthrough", "Scene", "scene app"});
    tails.emplace_back(logs / "xrservice.txt", std::vector<std::string>{"Passthrough cameras", "Room View", "RoomView", "roomview"});
    tails.emplace_back(logs / "vrclient_vrdashboard.txt", std::vector<std::string>{"RoomView", "roomview", "assthrough"});

    emit("START", "fp-probe pid=" + std::to_string(getpid()) + " interval_ms=" + std::to_string(interval_ms));
    while (!stopping) {
        vr::EVRInitError error = vr::VRInitError_None;
        vr::VR_Init(&error, vr::VRApplication_Background);
        if (error != vr::VRInitError_None) {
            static vr::EVRInitError last = vr::VRInitError_None;
            if (error != last) emit("WAIT", vr::VR_GetVRInitErrorAsEnglishDescription(error));
            last = error;
            for (int i = 0; i < 50 && !stopping; ++i) {
                for (auto& tail : tails) tail.poll();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            continue;
        }
        emit("CONNECTED", std::string("runtime ") + (vr::VRSystem() ? vr::VRSystem()->GetRuntimeVersion() : "?"));
        State last;
        bool quit = false;
        std::string repeat_key;
        uint32_t repeat_count = 0;
        auto repeat_since = std::chrono::steady_clock::now();
        while (!stopping && !quit) {
            vr::VREvent_t event{};
            while (vr::VRSystem()->PollNextEvent(&event, sizeof(event))) {
                // Collapse floods of one event type (e.g. property changes).
                const auto now = std::chrono::steady_clock::now();
                const auto key = std::to_string(event.eventType) + ":" + std::to_string(event.trackedDeviceIndex) + ":" + hex(&event.data, 16);
                if (key == repeat_key && now - repeat_since < std::chrono::seconds(1)) {
                    ++repeat_count;
                } else {
                    if (repeat_count) emit("EVT", "  (previous event repeated " + std::to_string(repeat_count) + "x)");
                    repeat_key = key;
                    repeat_count = 0;
                    repeat_since = now;
                    emit("EVT", describe_event(event));
                }
                if (event.eventType == vr::VREvent_Quit) {
                    vr::VRSystem()->AcknowledgeQuit_Exiting();
                    quit = true;
                }
            }
            if (quit) break;
            const auto state = read_state();
            std::string changes;
            for (const auto& [key, value] : state) {
                const auto it = last.find(key);
                if (it == last.end() || it->second != value) changes += " " + key + "=" + value;
            }
            if (!changes.empty()) emit(last.empty() ? "STATE0" : "STATE", changes.substr(1));
            last = state;
            for (auto& tail : tails) tail.poll();
            std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
        }
        vr::VR_Shutdown();
        emit("DISCONNECTED", quit ? "SteamVR quit" : "stopping");
        if (quit) std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    emit("STOP", "fp-probe exiting");
    return 0;
}
