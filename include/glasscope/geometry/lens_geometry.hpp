#pragma once
#include "glasscope/geometry/lens_bounds.hpp"
#include "glasscope/geometry/pinned_shape.hpp"
#include "glasscope/model/lens_config.hpp"
#include "glasscope/model/lens_snapshot.hpp"
namespace Glasscope {
[[nodiscard]] bool lensContains(Vec2 point, const LensSnapshot& snapshot, const LensStyle& style,
                                double scale) noexcept;
[[nodiscard]] double interactionStretch(double amount, const LensStyle& style, bool pinned) noexcept;
[[nodiscard]] double interactionStretch(float amount, const LensStyle& style, bool pinned) noexcept;
[[nodiscard]] double effectiveLensRadius(double radius, double reveal, double wobble, double motion) noexcept;
}
