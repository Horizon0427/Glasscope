#pragma once

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

} // namespace LensLimits

struct LensStyle {
    float radius = 190.0F;
    float zoom = 2.0F;
    float refraction = 1.0F;
    float dispersion = 0.7F;
    float bulge = 0.08F;
    float edgeWidth = 22.0F;
    float edgeStrength = 1.25F;
    float motionStrength = 1.5F;
    bool nearest = false;
};

struct GlasscopeConfig {
    bool enabled = false;
    LensStyle style;
};

} // namespace Glasscope
