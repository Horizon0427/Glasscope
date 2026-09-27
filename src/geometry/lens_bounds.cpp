#include "glasscope/geometry/lens_bounds.hpp"
#include <algorithm>
namespace Glasscope {
double lensExtent(const LensGeometry& geometry) noexcept {
    const double trailLength = std::min(geometry.maximumTrailLength * geometry.motionStrength, geometry.radius * 2.6);
    const double padding = 28.0 + geometry.edgeWidth * 0.25 +
                           geometry.edgeStrength * (geometry.refraction * 22.0 + geometry.dispersion * 8.0) +
                           trailLength * 1.10;
    return std::max(geometry.radius + padding, geometry.radius * 1.35 + 4.0);
}

namespace {
double extentWithTrail(const LensStyle& style, double radius, double edgeWidth, double trail) noexcept {
    return lensExtent({.radius = radius,
                       .edgeWidth = edgeWidth,
                       .edgeStrength = style.edgeStrength,
                       .refraction = style.refraction,
                       .dispersion = style.dispersion,
                       .motionStrength = style.motionStrength,
                       .maximumTrailLength = trail});
}
}
double lensExtent(const LensStyle& style, double radius, double edgeWidth, const std::array<Vec2, 3>& nodes) noexcept {
    double trail = 0.0;
    for (Vec2 node : nodes)
        trail = std::max(trail, length(node));
    return extentWithTrail(style, radius, edgeWidth, trail);
}
double lensExtent(const LensStyle& style, double radius, double edgeWidth, const std::array<float, 6>& nodes) noexcept {
    // Preserve the renderer's float rounding before constructing its pixel bounds.
    float trail = 0.0F;
    for (std::size_t i = 0; i < nodes.size(); i += 2)
        trail = std::max(trail, std::hypot(nodes[i], nodes[i + 1]));
    return extentWithTrail(style, radius, edgeWidth, trail);
}
}
