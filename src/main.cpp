#include "config.hpp"
#include "controller.hpp"
#include "gate.hpp"
#include "passthrough_state.hpp"
#include "openvr_camera_source.hpp"
#include <openvr.h>
#include <json.hpp>
#include <algorithm>
#include <array>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <cerrno>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <unistd.h>

namespace fs = std::filesystem;
using json = nlohmann::json;
namespace {
constexpr const char* app_key = "local.frame.passthrough.shortcuts";
constexpr const char* service_name = "frame-passthrough-shortcuts.service";
constexpr const char* devkit_game_id = "Frame_Passthrough_Shortcuts";
volatile std::sig_atomic_t stopping = 0;
std::ofstream application_log;
void stop(int) { stopping = 1; }
std::string wall_clock() {
    const auto now = std::chrono::system_clock::now();
    const auto seconds = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
    localtime_r(&seconds, &local);
    char text[32];
    std::strftime(text, sizeof(text), "%Y-%m-%d %H:%M:%S", &local);
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;
    char full[40];
    std::snprintf(full, sizeof(full), "%s.%03d", text, static_cast<int>(ms));
    return full;
}
void log(const std::string& message) {
    std::cout << message << std::endl;
    // Timestamps let application.log be lined up with SteamVR's own logs.
    if (application_log.is_open()) application_log << wall_clock() << ' ' << message << std::endl;
}

fs::path executable() {
    return fs::canonical("/proc/self/exe");
}
fs::path config_home() {
    if (const auto xdg = std::getenv("XDG_CONFIG_HOME"); xdg && *xdg) {
        if (!fs::path(xdg).is_absolute()) throw std::runtime_error("XDG_CONFIG_HOME must be absolute");
        return xdg;
    }
    const auto base = std::getenv("HOME");
    if (!base || !*base) throw std::runtime_error("HOME is unavailable");
    return fs::path(base) / ".config";
}
fs::path app_directory() { return config_home() / "frame-passthrough-shortcuts"; }

fs::path steamvr_server_log() {
    const auto home = std::getenv("HOME");
    const fs::path base = home && *home ? fs::path(home) : fs::path("/");
    std::error_code error;
    const auto root = fs::canonical(base / ".steam/root", error);
    const auto logs = !error && fs::is_directory(root / "logs") ? root / "logs" : base / ".local/share/Steam/logs";
    return logs / "vrserver.txt";
}

uint64_t monotonic_ms() {
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

void write_file(const fs::path& path, const std::string& contents) {
    fs::create_directories(path.parent_path());
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!(stream << contents) || !stream.flush())
        throw std::runtime_error("Cannot write " + path.string());
}

class InstanceLock {
    int fd_ = -1;
public:
    InstanceLock() {
        fs::create_directories(app_directory());
        fd_ = ::open((app_directory() / "instance.lock").c_str(), O_CREAT | O_RDWR | O_CLOEXEC, 0600);
        if (fd_ < 0) throw std::runtime_error("Cannot open instance lock");
        if (::flock(fd_, LOCK_EX | LOCK_NB) < 0) {
            ::close(fd_); fd_ = -1;
            throw std::runtime_error("Shortcuts are already running (or instance lock unavailable). Use --stop before diagnostics or one-shot actions.");
        }
    }
    ~InstanceLock() {
        if (fd_ >= 0) ::close(fd_);
    }
};

class VrSession {
public:
    explicit VrSession(bool wait, bool utility = false, bool overlay = false) {
        unsigned attempts = 0;
        while (!stopping) {
            vr::EVRInitError error = vr::VRInitError_None;
            vr::VR_Init(&error, utility ? vr::VRApplication_Utility : overlay ? vr::VRApplication_Overlay : vr::VRApplication_Background);
            if (error == vr::VRInitError_None) {
                if (vr::VRApplications() && (utility || (vr::VRSystem() && vr::VRInput() && vr::VRSettings()))) return;
                vr::VR_Shutdown();
                throw std::runtime_error("SteamVR lacks a required OpenVR interface. Update SteamVR to use this build.");
            }
            const std::string reason = vr::VR_GetVRInitErrorAsEnglishDescription(error);
            if (!wait) throw std::runtime_error("SteamVR unavailable: " + reason);
            if (attempts++ % 6 == 0) log("Waiting for SteamVR: " + reason);
            for (int i = 0; i < 50 && !stopping; ++i)
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        throw std::runtime_error("Stopped while waiting for SteamVR");
    }
    ~VrSession() { vr::VR_Shutdown(); }
    VrSession(const VrSession&) = delete;
};

class VrSettings final : public fps::Settings {
    static void check(vr::EVRSettingsError error, const std::string& section, const std::string& key) {
        if (error != vr::VRSettingsError_None)
            throw std::runtime_error(section + "." + key + ": " + vr::VRSettings()->GetSettingsErrorNameFromEnum(error));
    }
public:
    fps::Value read(const std::string& section, const std::string& key, const fps::Value& hint) override {
        vr::EVRSettingsError error = vr::VRSettingsError_None;
        fps::Value result;
        if (std::holds_alternative<bool>(hint))
            result = vr::VRSettings()->GetBool(section.c_str(), key.c_str(), &error);
        else if (std::holds_alternative<int32_t>(hint))
            result = vr::VRSettings()->GetInt32(section.c_str(), key.c_str(), &error);
        else {
            std::array<char, 4096> value{};
            vr::VRSettings()->GetString(section.c_str(), key.c_str(), value.data(), static_cast<uint32_t>(value.size()), &error);
            result = std::string(value.data());
        }
        check(error, section, key);
        return result;
    }
    void write(const std::string& section, const std::string& key, const fps::Value& value) override {
        vr::EVRSettingsError error = vr::VRSettingsError_None;
        if (const auto v = std::get_if<bool>(&value)) vr::VRSettings()->SetBool(section.c_str(), key.c_str(), *v, &error);
        else if (const auto integer = std::get_if<int32_t>(&value)) vr::VRSettings()->SetInt32(section.c_str(), key.c_str(), *integer, &error);
        else vr::VRSettings()->SetString(section.c_str(), key.c_str(), std::get<std::string>(value).c_str(), &error);
        check(error, section, key);
    }
};

void cleanup_removed_install(const fs::path& binary);
void remove_openvr_user_state();

void check_app(vr::EVRApplicationError error, const char* action) {
    if (error != vr::VRApplicationError_None)
        throw std::runtime_error(std::string(action) + ": " + vr::VRApplications()->GetApplicationsErrorNameFromEnum(error));
}
void check_input(vr::EVRInputError error, const char* action) {
    if (error != vr::VRInputError_None)
        throw std::runtime_error(std::string(action) + " failed, SteamVR input error " + std::to_string(error));
}
void register_app(const fs::path& binary, const fs::path& assets, const fs::path& config, bool attach = true) {
    // Frame's manifest argument parser keeps quote characters literally. Store
    // the selected path in JSON so spaces and quotes never cross that parser.
    write_file(app_directory() / "launch.json", json{{"config", config.generic_u8string()}}.dump(2));
    json app = {
        {"app_key", app_key}, {"launch_type", "binary"},
        {"is_dashboard_overlay", true},
        // Keep SteamVR from mirroring this background helper into the Steam
        // library as a launchable title.
        {"is_internal", true},
        {"action_manifest_path", (assets / "actions.json").generic_u8string()},
        {"arguments", "--run --registered-config"},
        {"strings", {{"en_us", {{"name", "Frame Passthrough Shortcuts"},
          {"description", "Passthrough shortcuts; no overlay surface is rendered"}}}}}
    };
    app["binary_path_linux"] = binary.generic_u8string();
    // Frame's native ARM runtime selects this key, not binary_path_linux.
    app["binary_path_linux_arm"] = binary.generic_u8string();
    const auto pid = static_cast<uint32_t>(getpid());
    const auto manifest = app_directory() / "app.vrmanifest";
    write_file(manifest, json{{"source", "builtin"}, {"applications", json::array({app})}}.dump(2));
    check_app(vr::VRApplications()->AddApplicationManifest(manifest.u8string().c_str(), false), "Register application");
    if (!attach) return;
    std::array<char, vr::k_unMaxApplicationKeyLength> existing_key{};
    const auto identity_error = vr::VRApplications()->GetApplicationKeyByProcessId(pid, existing_key.data(), static_cast<uint32_t>(existing_key.size()));
    if (identity_error != vr::VRApplicationError_None || std::string(existing_key.data()) != app_key)
        check_app(vr::VRApplications()->IdentifyApplication(pid, app_key), "Identify shortcut application");
    check_input(vr::VRInput()->SetActionManifestPath((assets / "actions.json").u8string().c_str()), "Load actions");
}

constexpr std::array<const char*, 6> action_names = {"toggle_style", "translucent", "opaque", "toggle_rgb", "rgb", "monochrome"};
constexpr std::array<fps::Action, 6> action_kinds = {fps::Action::ToggleStyle, fps::Action::Translucent, fps::Action::Opaque,
    fps::Action::ToggleRgb, fps::Action::Rgb, fps::Action::Monochrome};

fps::Action parse_action(std::string name) {
    for (auto& c : name) if (c == '-') c = '_';
    for (size_t i = 0; i < action_names.size(); ++i) if (name == action_names[i]) return action_kinds[i];
    throw std::runtime_error("Unknown action: " + name);
}

void load_camera_preference(fps::Controller& controls) {
    const auto path = app_directory() / "camera-preference.json";
    if (!fs::exists(path)) return;
    try {
        std::ifstream stream(path);
        json saved;
        if (!(stream >> saved) || !saved.at("rgb").is_boolean()) throw std::runtime_error("expected Boolean rgb");
        controls.remember_rgb(saved.at("rgb").get<bool>());
    } catch (const std::exception& error) {
        log(std::string("Cannot load saved camera preference: ") + error.what());
    }
}

void save_camera_preference(const fps::Controller& controls) {
    if (!controls.preferred_rgb()) return;
    const auto temporary = app_directory() / "camera-preference.pending.json";
    write_file(temporary, json{{"rgb", *controls.preferred_rgb()}}.dump());
    fs::rename(temporary, app_directory() / "camera-preference.json");
}

int run(const fs::path& binary, const fs::path& assets, const fs::path& config_path, const fps::Config& config, bool bindings) {
    InstanceLock lock;
    application_log.open(app_directory() / "application.log", std::ios::app);
    VrSession session(!bindings, false, true);
    register_app(binary, assets, config_path);
    VrSettings settings;
    fps::OpenVrCameraSource camera;
    fps::Controller controls(settings, config, &camera);
    load_camera_preference(controls);
    vr::VRActionSetHandle_t action_set = vr::k_ulInvalidActionSetHandle;
    check_input(vr::VRInput()->GetActionSetHandle("/actions/shortcuts", &action_set), "Find action set");
    if (bindings) {
        check_input(vr::VRInput()->OpenBindingUI(app_key, action_set, vr::k_ulInvalidInputValueHandle, false), "Open bindings");
        return 0;
    }
    std::array<vr::VRActionHandle_t, action_names.size()> handles{};
    std::array<fps::PressGate, action_names.size()> gates;
    for (size_t i = 0; i < handles.size(); ++i) {
        const auto path = std::string("/actions/shortcuts/in/") + action_names[i];
        check_input(vr::VRInput()->GetActionHandle(path.c_str(), &handles[i]), "Find action");
        gates[i] = fps::PressGate(config.debounce_ms);
    }
    const auto pid = vr::VRApplications()->GetApplicationProcessId(app_key);
    if (!config.rgb.enabled) log("RGB / monochrome switching is disabled in configuration.");
    else if (config.rgb.backend == "steamvr_camera") {
        try {
            const auto current = camera.read();
            if (current[0] != 1) throw std::runtime_error("camera rendering is not initialized yet");
            log(std::string("Steam Frame camera source control ready: ") + (current[2] ? "RGB" : "monochrome"));
        } catch (const std::exception& error) {
            log(std::string("Camera source control is not ready: ") + error.what() + ". Actions will retry.");
        }
    }
    // Gestures act only while passthrough is visible. See passthrough_state.hpp
    // for the signals; the decision itself is the pure fps::InputGate.
    fps::PassthroughState passthrough(*fps::parse_passthrough_signal(config.passthrough_signal), steamvr_server_log());
    fps::InputGate gate;
    std::string gate_detail;
    vr::VRActiveActionSet_t active{};
    active.ulActionSet = action_set;
    active.nPriority = vr::k_nActionSetOverlayGlobalPriorityMax;
    // Respect the user's global-input setting. On the tested Frame runtime,
    // even the highest public priority cannot override dashboard input focus.
    log("Ready as VRApplication_Overlay. No overlay surface is created or rendered.");
    log("Overlay input priority configured; SteamVR dashboard focus can still block actions.");
    bool warned_inactive = false;
    std::string camera_error;
    std::string input_error;
    json last_status;
    const auto started = std::chrono::steady_clock::now();
    auto removal_seen = std::chrono::steady_clock::time_point{};
    while (!stopping) {
        vr::VREvent_t event{};
        while (vr::VRSystem()->PollNextEvent(&event, sizeof(event))) {
            if (event.eventType == vr::VREvent_Quit) {
                vr::VRSystem()->AcknowledgeQuit_Exiting();
                log("SteamVR is shutting down.");
                return 0;
            }
            if (event.eventType == vr::VREvent_ApplicationListUpdated &&
                !vr::VRApplications()->IsApplicationInstalled(app_key) && removal_seen == std::chrono::steady_clock::time_point{})
                removal_seen = std::chrono::steady_clock::now();
            // SteamVR broadcasts binding results for every application. The
            // low 32 bits of the app container are the owning process ID, so
            // ignore failures that belong to other apps (e.g. steam.client).
            if (event.eventType == vr::VREvent_Input_BindingLoadFailed &&
                static_cast<uint32_t>(event.data.inputBinding.ulAppContainer & 0xffffffffu) == static_cast<uint32_t>(getpid()))
                log("SteamVR could not load the selected bindings. Select a new binding in SteamVR.");
            if (event.eventType == vr::VREvent_RoomViewShown) passthrough.on_room_view_event(true);
            if (event.eventType == vr::VREvent_RoomViewHidden) passthrough.on_room_view_event(false);
        }
        passthrough.poll();
        {
            fps::GateInputs inputs;
            inputs.passthrough = passthrough.visibility();
            const bool was_open = gate.open();
            if (gate.update(inputs, monotonic_ms())) {
                log(std::string("Shortcuts ") + (gate.open() ? "enabled" : "disabled") + ": " +
                    fps::to_string(gate.reason()) + " (" + passthrough.detail() + ")");
                if (was_open && !gate.open())
                    for (auto& press : gates) press.reset();
            }
            gate_detail = passthrough.detail();
        }
        const bool removed_from_steamvr = !vr::VRApplications()->IsApplicationInstalled(app_key);
        const bool install_files_missing = !fs::exists(binary);
        if (removed_from_steamvr || install_files_missing) {
            if (removal_seen == std::chrono::steady_clock::time_point{}) removal_seen = std::chrono::steady_clock::now();
            // FrameDrop replaces a title by removing its directory before copying
            // the update. Wait out that short gap, then clean residual registration.
            if (std::chrono::steady_clock::now() - removal_seen >= std::chrono::seconds(15)) {
                log("SteamVR registration or installed files were removed; cleaning the remaining application data.");
                cleanup_removed_install(binary);
                return 0;
            }
        } else {
            removal_seen = std::chrono::steady_clock::time_point{};
        }
        try {
            if (controls.reconcile_camera_source()) log("Updated camera source for camera availability and saved preference.");
            camera_error.clear();
        } catch (const std::exception& error) {
            if (camera_error != error.what()) log(std::string("Camera restoration deferred: ") + error.what());
            camera_error = error.what();
        }
        const auto update_error = vr::VRInput()->UpdateActionState(&active, sizeof(active), 1);
        if (update_error != vr::VRInputError_None) {
            for (auto& gate : gates) gate.reset();
            const auto message = "SteamVR input update deferred, error " + std::to_string(update_error);
            if (input_error != message) log(message);
            input_error = message;
            std::this_thread::sleep_for(std::chrono::milliseconds(config.poll_interval_ms));
            continue;
        }
        bool any_active = false;
        bool read_failed = false;
        json input_active = json::object();
        const auto now = std::chrono::steady_clock::now();
        const auto ms = monotonic_ms();
        for (size_t i = 0; i < handles.size(); ++i) {
            vr::InputDigitalActionData_t data{};
            const auto error = vr::VRInput()->GetDigitalActionData(handles[i], &data, sizeof(data), vr::k_ulInvalidInputValueHandle);
            if (error != vr::VRInputError_None) {
                gates[i].reset();
                read_failed = true;
                const auto message = std::string("SteamVR action read deferred: ") + action_names[i] +
                    ", error " + std::to_string(error);
                if (input_error != message) log(message);
                input_error = message;
                input_active[action_names[i]] = false;
                continue;
            }
            any_active |= data.bActive;
            input_active[action_names[i]] = data.bActive;
            if (data.bActive && data.bState && data.bChanged)
                log(std::string("Input press: ") + action_names[i]);
            if (gates[i].update(data.bActive, data.bState, data.bChanged, ms)) {
                if (!gate.open()) {
                    log(std::string("Ignored ") + action_names[i] + ": " + fps::to_string(gate.reason()));
                    continue;
                }
                try { log(controls.apply(action_kinds[i])); save_camera_preference(controls); }
                catch (const std::exception& e) { log(std::string("Action failed: ") + e.what()); }
            }
        }
        if (!read_failed) input_error.clear();
        json status{{"pid", pid}, {"ready", true}, {"input_active", input_active},
            {"dashboard_visible", vr::VROverlay()->IsDashboardVisible()}, {"input_priority", active.nPriority}};
        try { status["arcturus_rgb_camera_connected"] = camera.rgb_available(); }
        catch (const std::exception& error) {
            status["arcturus_rgb_camera_connected"] = nullptr;
            if (camera_error != error.what()) log(std::string("Camera detection deferred: ") + error.what());
            camera_error = error.what();
        }
        if (controls.preferred_rgb()) status["preferred_rgb"] = *controls.preferred_rgb();
        status["gate"] = {{"open", gate.open()}, {"reason", fps::to_string(gate.reason())},
            {"passthrough", fps::to_string(passthrough.visibility())}, {"signal", gate_detail}};
        if (status != last_status) {
            log(std::string("Input state: ") + status.dump());
            write_file(app_directory() / "runtime.json", status.dump()); last_status = std::move(status);
        }
        if (!any_active && !warned_inactive && now - started > std::chrono::seconds(30)) {
            log("No active bindings. Choose Frame Passthrough Shortcuts in SteamVR controller bindings.");
            warned_inactive = true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(config.poll_interval_ms));
    }
    return 0;
}

int run_process(std::vector<std::string> arguments) {
    if (arguments.empty()) throw std::runtime_error("Cannot start an empty command");
    std::vector<char*> argv;
    for (auto& arg : arguments) argv.push_back(arg.data());
    argv.push_back(nullptr);
    const auto child = fork();
    if (child < 0) throw std::runtime_error("Cannot start " + arguments.front());
    if (child == 0) { execvp(argv[0], argv.data()); _exit(127); }
    int status = 0;
    while (waitpid(child, &status, 0) < 0) if (errno != EINTR) throw std::runtime_error("Cannot wait for " + arguments.front());
    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}
int systemctl(const std::vector<std::string>& arguments) {
    std::vector<std::string> all{"systemctl", "--user"};
    all.insert(all.end(), arguments.begin(), arguments.end());
    return run_process(all);
}
void require_systemctl(const std::vector<std::string>& arguments) {
    if (systemctl(arguments) != 0) throw std::runtime_error("systemctl --user failed. Run --run in a SteamVR session or inspect journalctl --user -u " + std::string(service_name));
}
void remove_legacy_service() {
    const auto unit = config_home() / "systemd/user" / service_name;
    if (!fs::exists(unit)) return;
    require_systemctl({"disable", "--now", service_name});
    fs::remove(unit);
    require_systemctl({"daemon-reload"});
}

fs::path managed_install_directory() {
    const auto home = std::getenv("HOME");
    if (!home || !*home) throw std::runtime_error("HOME is unavailable");
    return fs::path(home) / "devkit-game" / devkit_game_id;
}

void remove_devkit_shortcut() {
    const auto home = std::getenv("HOME");
    if (!home || !*home) throw std::runtime_error("HOME is unavailable");
    const auto helper = fs::path(home) / "devkit-utils" / "steamos-delete";
    if (!fs::is_regular_file(helper)) {
        log("Steam library shortcut cleanup helper is unavailable; remove the Devkit Game entry from Steam manually.");
        return;
    }
    if (run_process({"python3", helper.string(), "--delete-title", devkit_game_id}) != 0)
        throw std::runtime_error("SteamOS could not remove the Devkit Game shortcut");
}

void remove_application_registration(const fs::path& manifest) {
    if (vr::VRApplications()->IsApplicationInstalled(app_key)) {
        if (vr::VRApplications()->GetApplicationPropertyBool(app_key, vr::VRApplicationProperty_IsDashboardOverlay_Bool))
            check_app(vr::VRApplications()->SetApplicationAutoLaunch(app_key, false), "Disable SteamVR autostart");
        check_app(vr::VRApplications()->RemoveApplicationManifest(manifest.u8string().c_str()), "Unregister application");
    }
    std::error_code error;
    fs::remove(manifest, error);
    if (error) throw std::runtime_error("Cannot remove the SteamVR application manifest: " + error.message());
}

void cleanup_removed_install(const fs::path& binary) {
    remove_legacy_service();
    remove_application_registration(app_directory() / "app.vrmanifest");
    remove_devkit_shortcut();
    remove_openvr_user_state();
    const auto home = std::getenv("HOME");
    if (!home || !*home) throw std::runtime_error("HOME is unavailable");
    const auto devkit_root = fs::path(home) / "devkit-game";
    for (const auto* suffix : {"-argv.json", "-settings.json"}) {
        std::error_code sidecar_error;
        fs::remove(devkit_root / (std::string(devkit_game_id) + suffix), sidecar_error);
        if (sidecar_error) throw std::runtime_error("Cannot remove Devkit Game metadata: " + sidecar_error.message());
    }
    std::error_code error;
    fs::remove_all(app_directory(), error);
    if (error) throw std::runtime_error("Cannot remove Frame Passthrough Shortcuts settings: " + error.message());
    const auto managed = managed_install_directory();
    std::error_code parent_error, expected_error;
    const auto parent = fs::weakly_canonical(binary.parent_path(), parent_error);
    const auto expected = fs::weakly_canonical(managed, expected_error);
    if (!parent_error && !expected_error && parent == expected) {
        fs::remove_all(managed, error);
        if (error) throw std::runtime_error("Cannot remove Frame Passthrough Shortcuts files: " + error.message());
    }
    log("Removed SteamVR registration, autostart, Devkit shortcut, and application files.");
}

void remove_openvr_user_state() {
    const auto openvr_config = config_home() / "openvr" / "config";
    std::error_code error;
    fs::remove(openvr_config / "vrappconfig" / "local.frame.passthrough.shortcuts.vrappconfig", error);
    if (error) throw std::runtime_error("Cannot remove SteamVR app settings: " + error.message());

    const auto settings_path = openvr_config / "steamvr.vrsettings";
    if (fs::exists(settings_path)) {
        std::ifstream input(settings_path);
        json settings;
        if (!input || !(input >> settings) || !settings.is_object())
            throw std::runtime_error("Cannot parse SteamVR settings while removing this app's binding state");
        if (settings.erase(app_key) != 0) {
            const auto temporary = settings_path.string() + ".frame-passthrough-shortcuts.tmp";
            write_file(temporary, settings.dump(3) + "\n");
            fs::rename(temporary, settings_path);
        }
    }

    const auto home = std::getenv("HOME");
    if (!home || !*home) throw std::runtime_error("HOME is unavailable");
    const auto logs = fs::path(home) / ".local/share/Steam/logs";
    if (fs::is_directory(logs)) {
        for (const auto& entry : fs::directory_iterator(logs)) {
            const auto name = entry.path().filename().string();
            if (name.rfind("vrclient_frame-passthrough-shortcuts", 0) == 0 ||
                name.rfind("vrclient_frame-passthrough-control", 0) == 0) {
                fs::remove(entry.path(), error);
                if (error) throw std::runtime_error("Cannot remove this app's SteamVR logs: " + error.message());
            }
        }
    }
}

void stop_registered_application() {
    const auto pid = vr::VRApplications()->GetApplicationProcessId(app_key);
    if (!pid) return;
    std::array<char, 4096> registered_binary{};
    vr::EVRApplicationError error = vr::VRApplicationError_None;
    vr::VRApplications()->GetApplicationPropertyString(app_key, vr::VRApplicationProperty_BinaryPath_String,
        registered_binary.data(), static_cast<uint32_t>(registered_binary.size()), &error);
    check_app(error, "Read registered binary before stopping");
    const auto process = fs::path("/proc") / std::to_string(pid);
    std::error_code file_error;
    const auto running_binary = fs::read_symlink(process / "exe", file_error);
    if (file_error) {
        if (!fs::exists(process)) return;
        throw std::runtime_error("Cannot verify the shortcut process executable before stopping");
    }
    if (fs::weakly_canonical(running_binary) != fs::weakly_canonical(registered_binary.data()) || pid == static_cast<uint32_t>(getpid()))
        throw std::runtime_error("Refusing to stop a process that does not match the registered shortcut executable");
    if (::kill(static_cast<pid_t>(pid), SIGTERM) != 0 && errno != ESRCH)
        throw std::runtime_error("Cannot stop the registered shortcut process");
    for (int attempt = 0; attempt < 50; ++attempt) {
        if (!fs::exists(process) && vr::VRApplications()->GetApplicationProcessId(app_key) != pid) return;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    throw std::runtime_error("Shortcut process did not exit within five seconds");
}

json application_status() {
    const bool installed = vr::VRApplications()->IsApplicationInstalled(app_key);
    const bool overlay = installed && vr::VRApplications()->GetApplicationPropertyBool(app_key, vr::VRApplicationProperty_IsDashboardOverlay_Bool);
    const auto pid = installed ? vr::VRApplications()->GetApplicationProcessId(app_key) : 0;
    bool ready = false;
    if (pid) {
        std::ifstream stream(app_directory() / "runtime.json");
        try {
            json marker;
            if (stream >> marker) ready = marker.value("pid", uint32_t{0}) == pid && marker.value("ready", false);
        } catch (const json::exception&) {}
    }
    return {{"installed", installed}, {"autolaunch", overlay && vr::VRApplications()->GetApplicationAutoLaunch(app_key)},
        {"running", pid != 0}, {"pid", pid}, {"ready", ready}, {"application_type", overlay ? "overlay" : "background"}, {"renders_overlay", false}};
}

void verify_control_companion(const fs::path& binary, const fs::path& helper) {
    if (!fs::is_regular_file(helper) || fs::is_symlink(helper) || ::access(helper.c_str(), X_OK) != 0)
        throw std::runtime_error("Missing executable frame-passthrough-control companion; reinstall the complete bundle");
    if (fs::file_size(binary) != fs::file_size(helper))
        throw std::runtime_error("Control companion does not match this build; reinstall the complete bundle");
    std::ifstream primary(binary, std::ios::binary), control(helper, std::ios::binary);
    if (!primary || !control) throw std::runtime_error("Cannot read the control companion for verification");
    std::array<char, 16384> first{}, second{};
    while (primary) {
        primary.read(first.data(), first.size());
        control.read(second.data(), second.size());
        if (primary.gcount() != control.gcount() || !std::equal(first.begin(), first.begin() + primary.gcount(), second.begin()))
            throw std::runtime_error("Control companion does not match this build; reinstall the complete bundle");
    }
    if (primary.bad() || control.bad()) throw std::runtime_error("Cannot verify the control companion");
}

void help() {
    log("Frame Passthrough Shortcuts " FPS_VERSION "\n"
        "Creates no overlay surface. Switches translucent / opaque passthrough and\n"
        "Steam Frame RGB / monochrome camera selection.\n\n"
        "Default: Register and start SteamVR autolaunch.\n"
        "  --run                 Run an overlay application without creating an overlay surface\n"
        "  --install             Enable SteamVR autostart and launch shortcuts\n"
        "  --stop                Stop shortcuts for diagnostics; retain autostart\n"
        "  --status              Report registration, autostart and running PID as JSON\n"
        "  --uninstall           Remove SteamVR registration, shortcut, files, and settings\n"
        "  --doctor              Read runtime settings and report support; no setting changes\n"
        "  --self-test           Test style changes, then restore the original style\n"
        "  --bindings            Open bindings (use --stop first)\n"
        "  --action NAME         toggle-style, translucent, opaque, toggle-rgb, rgb, monochrome\n"
        "  --validate            Check config and bundled action manifest without SteamVR\n"
        "  --config PATH         Use this JSON config (default: config.json beside executable)\n"
        "  --help                Show this help");
}
}

int main(int argc, char** argv) {
    std::signal(SIGINT, stop);
    std::signal(SIGTERM, stop);
    try {
        const auto process_binary = executable();
        const bool control_process = process_binary.filename() == "frame-passthrough-control";
        const auto binary = control_process ? process_binary.parent_path() / "frame-passthrough-shortcuts" : process_binary;
        const auto assets = binary.parent_path();
        auto config_path = assets / "config.json";
        std::string command = "--install";
        bool explicit_command = false;
        bool registered_config = false;
        bool explicit_config = false;
        std::string action;
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--help" || arg == "-h") { help(); return 0; }
            if (arg == "--config") {
                if (++i == argc) throw std::runtime_error("--config needs a path");
                config_path = fs::absolute(argv[i]); explicit_config = true; continue;
            }
            if (arg == "--registered-config") { registered_config = true; continue; }
            if (explicit_command) throw std::runtime_error("Specify only one command");
            explicit_command = true; command = arg;
            if (arg == "--action") {
                if (++i == argc) throw std::runtime_error("--action needs a name");
                action = argv[i];
                (void)parse_action(action);
            } else if (arg != "--run" && arg != "--install" && arg != "--uninstall" && arg != "--stop" && arg != "--status" && arg != "--doctor" && arg != "--self-test" && arg != "--bindings" && arg != "--validate")
                throw std::runtime_error("Unknown command: " + arg);
        }
        const bool management = command == "--install" || command == "--uninstall" || command == "--stop" || command == "--status";
        if (control_process && !management)
            throw std::runtime_error("The control companion accepts management commands only");
        if (management) {
            // SteamVR assigns app identity by executable path even for Utility
            // sessions. A distinct real executable keeps status/install/stop
            // requests from taking over the resident application's PID.
            const auto helper = binary.parent_path() / "frame-passthrough-control";
            verify_control_companion(binary, helper);
            if (!control_process) {
                std::vector<char*> arguments;
                auto helper_text = helper.string();
                arguments.push_back(helper_text.data());
                for (int i = 1; i < argc; ++i) arguments.push_back(argv[i]);
                arguments.push_back(nullptr);
                ::execv(helper.c_str(), arguments.data());
                throw std::runtime_error("Cannot start frame-passthrough-control companion");
            }
        }
        if (command == "--status" || command == "--stop") {
            VrSession session(false, true);
            if (command == "--status") log(application_status().dump());
            else { stop_registered_application(); log("Shortcuts stopped. SteamVR autostart remains enabled."); }
            return 0;
        }
        if (command == "--uninstall") {
            VrSession session(false, true);
            stop_registered_application();
            cleanup_removed_install(binary);
            return 0;
        }
        if (registered_config) {
            if (explicit_config) throw std::runtime_error("Specify --config or --registered-config, not both");
            std::ifstream stream(app_directory() / "launch.json");
            json launch;
            if (!stream || !(stream >> launch)) throw std::runtime_error("Missing saved launch configuration; run --install again");
            config_path = fs::u8path(launch.at("config").get<std::string>());
            if (!config_path.is_absolute()) throw std::runtime_error("Saved launch configuration must use an absolute path");
        }
        auto config = fps::load_config(config_path);
        // Camera selection is automatic on Frame, including older config files.
        config.rgb.enabled = true;
        config.rgb.backend = "steamvr_camera";
        config.rgb.section.clear();
        config.rgb.key.clear();
        std::ifstream manifest_stream(assets / "actions.json");
        json manifest;
        if (!manifest_stream || !(manifest_stream >> manifest) || !manifest.contains("actions"))
            throw std::runtime_error("Missing or invalid bundled actions.json");
        if (command == "--validate") {
            for (const auto& binding : manifest.at("default_bindings")) {
                std::ifstream stream(assets / binding.at("binding_url").get<std::string>());
                json value;
                if (!stream || !(stream >> value) || !value.contains("bindings")) throw std::runtime_error("Missing or invalid default binding");
            }
            log("Configuration and action files are valid.");
            if (config.rgb.enabled && config.rgb.backend == "steamvr_camera")
                log("Steam Frame RGB / monochrome backend configured; run --doctor on the Frame to check support.");
            else log(config.rgb.enabled ? "RGB backend configured; hardware behavior is unverified." : "RGB switching disabled: no verified Arcturus RGB camera selector configured.");
            return 0;
        }
        if (command == "--install") {
            remove_legacy_service();
            VrSession session(false, true);
            register_app(binary, assets, config_path, false);
            check_app(vr::VRApplications()->SetApplicationAutoLaunch(app_key, true), "Enable SteamVR autostart");
            if (!vr::VRApplications()->GetApplicationAutoLaunch(app_key))
                throw std::runtime_error("SteamVR did not retain the autostart setting");
            if (!vr::VRApplications()->GetApplicationProcessId(app_key))
                check_app(vr::VRApplications()->LaunchDashboardOverlay(app_key), "Launch shortcuts");
            log("Registered for SteamVR overlay autostart. No overlay surface is created or rendered.");
            return 0;
        }
        if (command == "--self-test") {
            InstanceLock lock;
            VrSession session(false);
            VrSettings settings;
            log(fps::smoke_test(settings, config));
            return 0;
        }
        if (command == "--doctor" || command == "--action") {
            // SteamVR associates clients by executable path. A second client can
            // take the background app's PID, so serialize all live VR commands.
            InstanceLock lock;
            VrSession session(false);
            VrSettings settings;
            fps::OpenVrCameraSource camera;
            if (command == "--action") {
                fps::Controller controls(settings, config, &camera);
                load_camera_preference(controls);
                log(controls.apply(parse_action(action)));
                save_camera_preference(controls);
                return 0;
            }
            log("Connected as VRApplication_Background. No overlay created.");
            const auto style = settings.read("camera", "roomViewStyle", int32_t(0));
            log("camera.roomViewStyle=" + std::to_string(std::get<int32_t>(style)) + " (3=translucent, 4=opaque)");
            log("camera.roomView=" + std::to_string(std::get<int32_t>(settings.read("camera", "roomView", int32_t(0)))));
            if (config.rgb.enabled && config.rgb.backend == "steamvr_camera") {
                const auto current = camera.read();
                if (current[0] != 1) throw std::runtime_error("Steam Frame camera rendering is not initialized; open passthrough and retry");
                log(std::string("Steam Frame camera source: ") + (current[2] ? "RGB" : "monochrome"));
                log("Camera flags: stereo=" + std::to_string(current[1]) + ", disable_devignetting=" +
                    std::to_string(current[3]) + ", sharpening=" + std::to_string(current[4]));
            } else if (config.rgb.enabled) {
                const auto current = settings.read(config.rgb.section, config.rgb.key, config.rgb.rgb_value);
                if (current != config.rgb.rgb_value && current != config.rgb.monochrome_value)
                    throw std::runtime_error("Configured RGB setting has an unknown value; source switching is unavailable");
                log(current == config.rgb.rgb_value ? "Configured RGB source setting: RGB" : "Configured RGB source setting: monochrome");
            } else log("RGB / monochrome: UNSUPPORTED until a verified source selector is configured.");
            return 0;
        }
        return run(binary, assets, config_path, config, command == "--bindings");
    } catch (const std::exception& e) {
        if (stopping) return 0;
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
