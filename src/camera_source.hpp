#pragma once

#include <array>
#include <cstdint>

namespace fps {

// Runtime configuration: enabled, stereo, RGB source, disable devignetting,
// sharpening. Source selection must preserve every other byte.
using CameraConfig = std::array<std::uint8_t, 5>;

class CameraSource {
public:
    virtual ~CameraSource() = default;
    virtual bool rgb_available() { return true; }
    virtual CameraConfig read() = 0;
    virtual void write(const CameraConfig& config) = 0;
};

} // namespace fps
