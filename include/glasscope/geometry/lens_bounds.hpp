#pragma once
#include "glasscope/math/vector.hpp"
#include "glasscope/model/lens_config.hpp"
#include <array>
namespace Glasscope {
struct LensGeometry {
    double radius = 0.0;
    double edgeWidth = 0.0;
    double edgeStrength = 0.0;
    double refraction = 0.0;
    double dispersion = 0.0;
    double motionStrength = 0.0;
    double maximumTrailLength = 0.0;
};

[[nodiscard]] double lensExtent(const LensGeometry& geometry) noexcept;
// Inputs and fixed padding use the same unit: logical or physical pixels.
[[nodiscard]] double lensExtent(const LensStyle& style, double radius, double edgeWidth,
                                const std::array<Vec2, 3>& trailNodes) noexcept;
[[nodiscard]] double lensExtent(const LensStyle& style, double radius, double edgeWidth,
                                const std::array<float, 6>& trailNodes) noexcept;
}
