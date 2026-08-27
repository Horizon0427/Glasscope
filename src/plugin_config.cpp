#include "plugin_config.hpp"

#include <algorithm>
#include <stdexcept>

namespace Glasscope {

PluginConfig::PluginConfig(HANDLE handle) {
    using namespace Config::Values;

    const GlasscopeConfig defaults;
    m_enabled = makeShared<CBoolValue>("plugin:glasscope:enabled",
                                       "Master switch for Glasscope rendering and interaction", defaults.enabled);
    m_radius = makeShared<CIntValue>("plugin:glasscope:radius", "Lens radius in logical pixels",
                                     static_cast<int>(defaults.style.radius),
                                     SIntValueOptions{.min = static_cast<int>(LensLimits::RADIUS_MIN),
                                                      .max = static_cast<int>(LensLimits::RADIUS_MAX)});
    m_zoom = makeShared<CFloatValue>("plugin:glasscope:zoom", "Local magnification factor", defaults.style.zoom,
                                     SFloatValueOptions{.min = LensLimits::ZOOM_MIN,
                                                        .max = LensLimits::ZOOM_MAX});
    m_refraction = makeShared<CFloatValue>(
        "plugin:glasscope:refraction", "Strength of edge displacement and optical tint", defaults.style.refraction,
        SFloatValueOptions{.min = LensLimits::REFRACTION_MIN, .max = LensLimits::REFRACTION_MAX});
    m_dispersion = makeShared<CFloatValue>(
        "plugin:glasscope:dispersion", "Chromatic dispersion strength", defaults.style.dispersion,
        SFloatValueOptions{.min = LensLimits::DISPERSION_MIN, .max = LensLimits::DISPERSION_MAX});
    m_bulge = makeShared<CFloatValue>("plugin:glasscope:bulge", "Additional convex magnification near the lens center",
                                      defaults.style.bulge,
                                      SFloatValueOptions{.min = LensLimits::BULGE_MIN, .max = LensLimits::BULGE_MAX});
    m_edgeWidth = makeShared<CFloatValue>(
        "plugin:glasscope:edge_width", "Width of the optical edge in logical pixels", defaults.style.edgeWidth,
        SFloatValueOptions{.min = LensLimits::EDGE_WIDTH_MIN, .max = LensLimits::EDGE_WIDTH_MAX});
    m_edgeStrength = makeShared<CFloatValue>(
        "plugin:glasscope:edge_strength", "Master strength for edge refraction, tint, shade and highlight",
        defaults.style.edgeStrength,
        SFloatValueOptions{.min = LensLimits::EDGE_STRENGTH_MIN, .max = LensLimits::EDGE_STRENGTH_MAX});
    m_motionStrength = makeShared<CFloatValue>(
        "plugin:glasscope:motion_strength", "Strength of trailing, moving ripples and stop deformation",
        defaults.style.motionStrength,
        SFloatValueOptions{.min = LensLimits::MOTION_STRENGTH_MIN, .max = LensLimits::MOTION_STRENGTH_MAX});
    m_nearest = makeShared<CBoolValue>("plugin:glasscope:nearest",
                                       "Use nearest-neighbor sampling for pixel inspection", defaults.style.nearest);

    if (!HyprlandAPI::addConfigValueV2(handle, m_enabled) || !HyprlandAPI::addConfigValueV2(handle, m_radius) ||
        !HyprlandAPI::addConfigValueV2(handle, m_zoom) || !HyprlandAPI::addConfigValueV2(handle, m_refraction) ||
        !HyprlandAPI::addConfigValueV2(handle, m_dispersion) || !HyprlandAPI::addConfigValueV2(handle, m_bulge) ||
        !HyprlandAPI::addConfigValueV2(handle, m_edgeWidth) ||
        !HyprlandAPI::addConfigValueV2(handle, m_edgeStrength) ||
        !HyprlandAPI::addConfigValueV2(handle, m_motionStrength) ||
        !HyprlandAPI::addConfigValueV2(handle, m_nearest))
        throw std::runtime_error("glasscope: failed to register config values");

    HyprlandAPI::reloadConfig();
}

bool PluginConfig::enabled() const {
    return m_enabled && m_enabled->value();
}

GlasscopeConfig PluginConfig::snapshot() const {
    return {
        .enabled = enabled(),
        .style = {
            .radius = m_radiusOverride.value_or(static_cast<float>(m_radius->value())),
            .zoom = m_zoomOverride.value_or(m_zoom->value()),
            .refraction = m_refraction->value(),
            .dispersion = m_dispersion->value(),
            .bulge = m_bulge->value(),
            .edgeWidth = m_edgeWidthOverride.value_or(m_edgeWidth->value()),
            .edgeStrength = m_edgeStrength->value(),
            .motionStrength = m_motionStrength->value(),
            .nearest = m_nearest->value(),
        },
    };
}

bool PluginConfig::hasOverrides() const {
    return m_radiusOverride || m_zoomOverride || m_edgeWidthOverride;
}

void PluginConfig::setZoomOverride(float value) {
    m_zoomOverride = std::clamp(value, LensLimits::ZOOM_MIN, LensLimits::ZOOM_MAX);
}

void PluginConfig::setRadiusOverride(float value) {
    m_radiusOverride = std::clamp(value, LensLimits::RADIUS_MIN, LensLimits::RADIUS_MAX);
}

void PluginConfig::setEdgeWidthOverride(float value) {
    m_edgeWidthOverride = std::clamp(value, LensLimits::EDGE_WIDTH_MIN, LensLimits::EDGE_WIDTH_MAX);
}

void PluginConfig::clearOverrides() {
    m_radiusOverride.reset();
    m_zoomOverride.reset();
    m_edgeWidthOverride.reset();
}

} // namespace Glasscope
