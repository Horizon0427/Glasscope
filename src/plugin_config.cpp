#include "plugin_config.hpp"

#include <algorithm>
#include <stdexcept>

namespace Glasscope {
namespace {

std::array<float, 4> colorChannels(Config::INTEGER value) {
    const CHyprColor color{static_cast<uint64_t>(static_cast<uint32_t>(value))};
    return {static_cast<float>(color.r), static_cast<float>(color.g), static_cast<float>(color.b),
            static_cast<float>(color.a)};
}

}

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
                                     SFloatValueOptions{.min = LensLimits::ZOOM_MIN, .max = LensLimits::ZOOM_MAX});
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
    m_interactionBounce = makeShared<CFloatValue>(
        "plugin:glasscope:interaction_bounce", "Surface squash strength for pinned clicks and drag rebounds",
        defaults.style.interactionBounce,
        SFloatValueOptions{.min = LensLimits::INTERACTION_BOUNCE_MIN, .max = LensLimits::INTERACTION_BOUNCE_MAX});
    m_nearest = makeShared<CBoolValue>("plugin:glasscope:nearest", "Use nearest-neighbor sampling for pixel inspection",
                                       defaults.style.nearest);
    m_colorStrength = makeShared<CFloatValue>(
        "plugin:glasscope:color_strength", "Custom colour strength; zero restores the original lens",
        defaults.style.colorStrength,
        SFloatValueOptions{.min = LensLimits::COLOR_STRENGTH_MIN, .max = LensLimits::COLOR_STRENGTH_MAX});
    m_colorWidth = makeShared<CFloatValue>(
        "plugin:glasscope:color_width", "Custom colour band width in logical pixels; independent of optical edges",
        defaults.style.colorWidth,
        SFloatValueOptions{.min = LensLimits::COLOR_WIDTH_MIN, .max = LensLimits::COLOR_WIDTH_MAX});
    m_transmissionColor =
        makeShared<CColorValue>("plugin:glasscope:colors:transmission",
                                "Transmitted-light color; alpha controls gentle channel absorption", 0x00000000);
    m_refractionColor =
        makeShared<CColorValue>("plugin:glasscope:colors:refraction",
                                "Inner refractive rim color; alpha controls its contribution", 0x00000000);
    m_reflectionColor =
        makeShared<CColorValue>("plugin:glasscope:colors:reflection",
                                "Grazing-angle rim reflection color; alpha controls its contribution", 0x00000000);
    m_highlightColor = makeShared<CColorValue>(
        "plugin:glasscope:colors:highlight",
        "Specular highlight tint; alpha blends its colour without changing the original lighting", 0x00000000);

    if (!HyprlandAPI::addConfigValueV2(handle, m_enabled) || !HyprlandAPI::addConfigValueV2(handle, m_radius) ||
        !HyprlandAPI::addConfigValueV2(handle, m_zoom) || !HyprlandAPI::addConfigValueV2(handle, m_refraction) ||
        !HyprlandAPI::addConfigValueV2(handle, m_dispersion) || !HyprlandAPI::addConfigValueV2(handle, m_bulge) ||
        !HyprlandAPI::addConfigValueV2(handle, m_edgeWidth) || !HyprlandAPI::addConfigValueV2(handle, m_edgeStrength) ||
        !HyprlandAPI::addConfigValueV2(handle, m_motionStrength) ||
        !HyprlandAPI::addConfigValueV2(handle, m_interactionBounce) ||
        !HyprlandAPI::addConfigValueV2(handle, m_colorStrength) ||
        !HyprlandAPI::addConfigValueV2(handle, m_colorWidth) ||
        !HyprlandAPI::addConfigValueV2(handle, m_transmissionColor) ||
        !HyprlandAPI::addConfigValueV2(handle, m_refractionColor) ||
        !HyprlandAPI::addConfigValueV2(handle, m_reflectionColor) ||
        !HyprlandAPI::addConfigValueV2(handle, m_highlightColor) || !HyprlandAPI::addConfigValueV2(handle, m_nearest))
        throw std::runtime_error("glasscope: failed to register config values");

    HyprlandAPI::reloadConfig();
}

bool PluginConfig::enabled() const {
    return m_enabled && m_enabled->value();
}

GlasscopeConfig PluginConfig::snapshot() const {
    return {
        .enabled = enabled(),
        .style = boundedLensStyle({
            .radius = m_radiusOverride.value_or(static_cast<float>(m_radius->value())),
            .zoom = m_zoomOverride.value_or(m_zoom->value()),
            .refraction = m_refraction->value(),
            .dispersion = m_dispersion->value(),
            .bulge = m_bulge->value(),
            .edgeWidth = m_edgeWidthOverride.value_or(m_edgeWidth->value()),
            .edgeStrength = m_edgeStrength->value(),
            .motionStrength = m_motionStrength->value(),
            .interactionBounce = m_interactionBounce->value(),
            .colorStrength = m_colorStrength->value(),
            .colorWidth = m_colorWidth->value(),
            .colors =
                {
                    .transmission = colorChannels(m_transmissionColor->value()),
                    .refraction = colorChannels(m_refractionColor->value()),
                    .reflection = colorChannels(m_reflectionColor->value()),
                    .highlight = colorChannels(m_highlightColor->value()),
                },
            .nearest = m_nearest->value(),
        }),
    };
}

bool PluginConfig::hasOverrides() const {
    return m_radiusOverride || m_zoomOverride || m_edgeWidthOverride;
}

void PluginConfig::setZoomOverride(float value) {
    m_zoomOverride = boundedLensValue(value, LensLimits::ZOOM_MIN, LensLimits::ZOOM_MAX, LensStyle{}.zoom);
}

void PluginConfig::setRadiusOverride(float value) {
    m_radiusOverride = boundedLensValue(value, LensLimits::RADIUS_MIN, LensLimits::RADIUS_MAX, LensStyle{}.radius);
}

void PluginConfig::setEdgeWidthOverride(float value) {
    m_edgeWidthOverride =
        boundedLensValue(value, LensLimits::EDGE_WIDTH_MIN, LensLimits::EDGE_WIDTH_MAX, LensStyle{}.edgeWidth);
}

void PluginConfig::clearOverrides() {
    m_radiusOverride.reset();
    m_zoomOverride.reset();
    m_edgeWidthOverride.reset();
}

}
