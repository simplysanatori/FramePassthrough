#pragma once

#include "camera_source.hpp"
#include <chrono>

namespace fps {

class OpenVrCameraSource final : public CameraSource {
public:
    bool rgb_available() override;
    CameraConfig read() override;
    void write(const CameraConfig& config) override;
private:
    std::chrono::steady_clock::time_point next_detection_{};
    std::chrono::steady_clock::time_point missing_since_{};
    bool detected_ = false;
};

} // namespace fps
