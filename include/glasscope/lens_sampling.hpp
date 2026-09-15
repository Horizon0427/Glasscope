#pragma once

namespace Glasscope {

inline constexpr float REDUCED_SAMPLE_MAX_RADIUS_PX = 224.0F;

[[nodiscard]] constexpr int lensBezierSamples(float physicalRadiusPx) noexcept {
    return physicalRadiusPx <= REDUCED_SAMPLE_MAX_RADIUS_PX ? 10 : 12;
}

}
