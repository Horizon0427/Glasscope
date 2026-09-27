#pragma once
#include "glasscope/math/vector.hpp"
namespace Glasscope {
// Metaball parameters normalized to the resting unit-disk area.
struct PinnedShape {
    double bodyRadius = 1.0;
    double pullRadius = 0.0;
    double separation = 0.0;
};
[[nodiscard]] PinnedShape pinnedShape(double strain, double share) noexcept;
[[nodiscard]] double pinnedDistance(Vec2 point, const PinnedShape& shape) noexcept;

}
