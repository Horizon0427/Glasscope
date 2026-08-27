#include "glasscope/lens_geometry.hpp"

#include <algorithm>

namespace Glasscope {

double lensExtent(const LensGeometry& geometry) noexcept {
    const double trailLength =
        std::min(geometry.maximumTrailLength * geometry.motionStrength, geometry.radius * 1.35);
    const double padding = 28.0 + geometry.edgeWidth * 0.25 +
                           geometry.edgeStrength * (geometry.refraction * 22.0 + geometry.dispersion * 8.0) +
                           trailLength * 0.90;
    return geometry.radius + padding;
}

} // namespace Glasscope
