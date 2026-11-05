#include "openvr_camera_source.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <filesystem>
#include <fstream>

#if defined(__linux__) && defined(__aarch64__)
#include <openvr.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace fps {
namespace {

void validate_camera_config(const CameraConfig& config) {
    if (!std::all_of(config.begin(), config.end(), [](std::uint8_t value) {
            return value <= 1;
        })) {
        throw std::runtime_error(
            "Steam Frame camera configuration contains invalid boolean values; "
            "the private SteamVR camera interface may have changed");
    }
}

#if defined(__linux__) && defined(__aarch64__)

// Verified on Steam Frame firmware build 20260918. This private interface may
// change with SteamVR updates. Slots 9/10 get/set exactly five bytes and use
// SteamVR's shared-state mutex; the setter notifies the compositor via event 812.
// The getter's bool is config[0] (override enabled), not a success/error value.
constexpr const char* kCameraInterface = "IVRCameraPassthroughInternal_001";
constexpr std::size_t kGetConfigSlot = 9;
constexpr std::size_t kSetConfigSlot = 10;

struct CameraInterface {
    void* instance;
    void** methods;
};

CameraInterface camera_interface() {
    vr::EVRInitError error = vr::VRInitError_None;
    void* instance = vr::VR_GetGenericInterface(kCameraInterface, &error);
    if (!instance || error != vr::VRInitError_None) {
        const char* description = error == vr::VRInitError_None
            ? "the runtime did not provide the camera interface"
            : vr::VR_GetVRInitErrorAsEnglishDescription(error);
        throw std::runtime_error(std::string("Steam Frame camera source is unavailable: ") +
            (description ? description : "unknown SteamVR interface error"));
    }

    auto** methods = *reinterpret_cast<void***>(instance);
    if (!methods || !methods[kGetConfigSlot] || !methods[kSetConfigSlot]) {
        throw std::runtime_error(
            "Steam Frame camera source is unavailable: the private SteamVR "
            "camera interface has no configuration accessors");
    }
    return {instance, methods};
}

#else

[[noreturn]] void camera_source_unavailable() {
    throw std::runtime_error(
        "Native Steam Frame camera source switching requires Linux ARM64; "
        "this platform supports diagnostics only");
}

#endif

} // namespace

bool OpenVrCameraSource::rgb_available() {
#if defined(__linux__) && defined(__aarch64__)
    const auto now = std::chrono::steady_clock::now();
    if (now < next_detection_) return detected_;
    next_detection_ = now + std::chrono::milliseconds(250);
    std::error_code error;
    auto node = std::filesystem::directory_iterator("/sys/class/video4linux", error);
    if (error) return detected_;
    const auto end = std::filesystem::directory_iterator();
    bool found_sensor = false;
    bool valid_read = false;
    bool any_connected = false;
    for (; node != end; node.increment(error)) {
        if (error) return detected_; // Hotplug can invalidate the directory while scanning.
        std::ifstream file(node->path() / "name");
        std::string name;
        std::getline(file, name);
        if (name.rfind("arcimx616 ", 0) != 0) continue;
        found_sensor = true;
        const auto device = std::filesystem::path("/dev") / node->path().filename();
        const int fd = ::open(device.c_str(), O_RDWR | O_NONBLOCK | O_CLOEXEC);
        if (fd < 0) continue;
        // Read-only physical connection query used by the camera driver.
        // Discover sensor nodes by name; their device numbers are not stable.
        std::uint32_t sensor_connected = 0;
        const int result = ::ioctl(fd, 0x800456c1UL, &sensor_connected);
        ::close(fd);
        if (result == 0) {
            valid_read = true;
            if (sensor_connected == 1) any_connected = true;
        }
    }
    if (error || (found_sensor && !valid_read)) return detected_;
    if (any_connected) {
        detected_ = true;
        missing_since_ = {};
    } else if (detected_) {
        // A transient zero must not drive repeated camera source transitions.
        if (missing_since_ == std::chrono::steady_clock::time_point{}) missing_since_ = now;
        if (now - missing_since_ >= std::chrono::seconds(2)) detected_ = false;
    }
    return detected_;
#else
    return false;
#endif
}

CameraConfig OpenVrCameraSource::read() {
#if defined(__linux__) && defined(__aarch64__)
    const auto camera = camera_interface();
    CameraConfig config;
    config.fill(0xff);
    using GetConfig = bool (*)(void*, std::uint8_t*);
    const auto get = reinterpret_cast<GetConfig>(camera.methods[kGetConfigSlot]);
    (void)get(camera.instance, config.data());
    validate_camera_config(config);
    return config;
#else
    camera_source_unavailable();
#endif
}

void OpenVrCameraSource::write(const CameraConfig& config) {
    validate_camera_config(config);
#if defined(__linux__) && defined(__aarch64__)
    const auto camera = camera_interface();
    using SetConfig = void (*)(void*, const std::uint8_t*);
    const auto set = reinterpret_cast<SetConfig>(camera.methods[kSetConfigSlot]);
    set(camera.instance, config.data());
#else
    camera_source_unavailable();
#endif
}

} // namespace fps
