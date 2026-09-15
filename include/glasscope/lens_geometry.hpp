#pragma once

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

}
