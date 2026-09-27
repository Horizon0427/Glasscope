#pragma once

namespace Glasscope {

struct LensSnapshot;
struct LensStyle;
struct Vec2;

struct LensGeometry {
    double radius = 0.0;
    double edgeWidth = 0.0;
    double edgeStrength = 0.0;
    double refraction = 0.0;
    double dispersion = 0.0;
    double motionStrength = 0.0;
    double maximumTrailLength = 0.0;
};

// Area-normalized metaball sources, shared by rendering and hit testing.
struct PinnedShape {
    double bodyRadius = 1.0;
    double pullRadius = 0.0;
    double separation = 0.0;
};
[[nodiscard]] PinnedShape pinnedShape(double strain, double share) noexcept;
[[nodiscard]] double pinnedDistance(Vec2 point, const PinnedShape& shape) noexcept;

[[nodiscard]] double lensExtent(const LensGeometry& geometry) noexcept;
[[nodiscard]] bool lensContains(Vec2 point, const LensSnapshot& snapshot, const LensStyle& style,
                                double scale) noexcept;

}
