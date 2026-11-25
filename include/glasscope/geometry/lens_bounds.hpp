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
// Half-width and half-height of the box around the lens: the round extent, grown
// along the pill axis (pillAxis is the half-length vector, same unit as the rest).
struct LensExtents {
    double x = 0.0;
    double y = 0.0;
};
[[nodiscard]] LensExtents lensExtents(const LensStyle& style, double radius, double edgeWidth,
                                      const std::array<Vec2, 3>& trailNodes, Vec2 pillAxis) noexcept;
[[nodiscard]] LensExtents lensExtents(const LensStyle& style, double radius, double edgeWidth,
                                      const std::array<float, 6>& trailNodes,
                                      const std::array<float, 2>& pillAxis) noexcept;
// Inputs and fixed padding use the same unit: logical or physical pixels.
[[nodiscard]] double lensExtent(const LensStyle& style, double radius, double edgeWidth,
                                const std::array<Vec2, 3>& trailNodes) noexcept;
[[nodiscard]] double lensExtent(const LensStyle& style, double radius, double edgeWidth,
                                const std::array<float, 6>& trailNodes) noexcept;
}
