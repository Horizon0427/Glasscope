#pragma once

#include <algorithm>
#include <array>
#include <cmath>

namespace Glasscope {

namespace LensLimits {

inline constexpr float RADIUS_MIN = 80.0F;
inline constexpr float RADIUS_MAX = 600.0F;
inline constexpr float ZOOM_MIN = 1.0F;
inline constexpr float ZOOM_MAX = 6.0F;
inline constexpr float REFRACTION_MIN = 0.0F;
inline constexpr float REFRACTION_MAX = 2.0F;
inline constexpr float DISPERSION_MIN = 0.0F;
inline constexpr float DISPERSION_MAX = 2.0F;
inline constexpr float BULGE_MIN = 0.0F;
inline constexpr float BULGE_MAX = 0.28F;
inline constexpr float EDGE_WIDTH_MIN = 4.0F;
inline constexpr float EDGE_WIDTH_MAX = 48.0F;
inline constexpr float EDGE_STRENGTH_MIN = 0.0F;
inline constexpr float EDGE_STRENGTH_MAX = 2.5F;
inline constexpr float MOTION_STRENGTH_MIN = 0.0F;
inline constexpr float MOTION_STRENGTH_MAX = 2.5F;
inline constexpr float INTERACTION_BOUNCE_MIN = 0.0F;
inline constexpr float INTERACTION_BOUNCE_MAX = 2.5F;
inline constexpr float COLOR_STRENGTH_MIN = 0.0F;
inline constexpr float COLOR_STRENGTH_MAX = 1.0F;
inline constexpr float COLOR_WIDTH_MIN = 4.0F;
inline constexpr float COLOR_WIDTH_MAX = 48.0F;

}

struct LensColors {
    std::array<float, 4> transmission = {};
    std::array<float, 4> refraction = {};
    std::array<float, 4> reflection = {};
    std::array<float, 4> highlight = {};
};

struct LensStyle {
    float radius = 190.0F;
    float zoom = 2.0F;
    float refraction = 1.0F;
    float dispersion = 0.7F;
    float bulge = 0.08F;
    float edgeWidth = 22.0F;
    float edgeStrength = 1.25F;
    float motionStrength = 1.5F;
    float interactionBounce = 1.0F;
    float colorStrength = 0.0F;
    float colorWidth = 18.0F;
    LensColors colors;
    bool nearest = false;
};

struct GlasscopeConfig {
    bool enabled = false;
    LensStyle style;
};

inline float boundedLensValue(float value, float minimum, float maximum, float fallback) {
    return std::isfinite(value) ? std::clamp(value, minimum, maximum) : fallback;
}

// Hyprland's config metadata does not enforce every direct write. Normalize
// values at the snapshot boundary before rendering, damage, or hit testing.
inline LensStyle boundedLensStyle(LensStyle style) {
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
