#include "glasscope/core/config_validation.hpp"
#include <algorithm>
#include <cmath>
namespace Glasscope {
float boundedLensValue(float value, float minimum, float maximum, float fallback) {
    return std::isfinite(value) ? std::clamp(value, minimum, maximum) : fallback;
}

// Hyprland metadata does not validate all direct writes.
LensStyle boundedLensStyle(LensStyle style) {
    const LensStyle defaults;
    using namespace LensLimits;
    style.radius = boundedLensValue(style.radius, RADIUS_MIN, RADIUS_MAX, defaults.radius);
    style.zoom = boundedLensValue(style.zoom, ZOOM_MIN, ZOOM_MAX, defaults.zoom);
    style.refraction = boundedLensValue(style.refraction, REFRACTION_MIN, REFRACTION_MAX, defaults.refraction);
    style.dispersion = boundedLensValue(style.dispersion, DISPERSION_MIN, DISPERSION_MAX, defaults.dispersion);
    style.bulge = boundedLensValue(style.bulge, BULGE_MIN, BULGE_MAX, defaults.bulge);
    style.edgeWidth = boundedLensValue(style.edgeWidth, EDGE_WIDTH_MIN, EDGE_WIDTH_MAX, defaults.edgeWidth);
    style.edgeStrength =
        boundedLensValue(style.edgeStrength, EDGE_STRENGTH_MIN, EDGE_STRENGTH_MAX, defaults.edgeStrength);
    style.motionStrength =
        boundedLensValue(style.motionStrength, MOTION_STRENGTH_MIN, MOTION_STRENGTH_MAX, defaults.motionStrength);
    style.interactionBounce = boundedLensValue(style.interactionBounce, INTERACTION_BOUNCE_MIN, INTERACTION_BOUNCE_MAX,
                                               defaults.interactionBounce);
    style.colorStrength =
        boundedLensValue(style.colorStrength, COLOR_STRENGTH_MIN, COLOR_STRENGTH_MAX, defaults.colorStrength);
    style.colorWidth = boundedLensValue(style.colorWidth, COLOR_WIDTH_MIN, COLOR_WIDTH_MAX, defaults.colorWidth);
    return style;
}

}
