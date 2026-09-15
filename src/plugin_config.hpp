#pragma once

#include "glasscope/lens_config.hpp"

#include <hyprland/src/config/values/types/BoolValue.hpp>
#include <hyprland/src/config/values/types/ColorValue.hpp>
#include <hyprland/src/config/values/types/FloatValue.hpp>
#include <hyprland/src/config/values/types/IntValue.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>

#include <optional>

namespace Glasscope {

class PluginConfig {
  public:
    explicit PluginConfig(HANDLE handle);

    [[nodiscard]] bool enabled() const;
    [[nodiscard]] GlasscopeConfig snapshot() const;
    [[nodiscard]] bool hasOverrides() const;

    void setZoomOverride(float value);
    void setRadiusOverride(float value);
    void setEdgeWidthOverride(float value);
    void clearOverrides();

  private:
    SP<Config::Values::CBoolValue> m_enabled;
    SP<Config::Values::CIntValue> m_radius;
    SP<Config::Values::CFloatValue> m_zoom;
    SP<Config::Values::CFloatValue> m_refraction;
    SP<Config::Values::CFloatValue> m_dispersion;
    SP<Config::Values::CFloatValue> m_bulge;
    SP<Config::Values::CFloatValue> m_edgeWidth;
    SP<Config::Values::CFloatValue> m_edgeStrength;
    SP<Config::Values::CFloatValue> m_motionStrength;
    SP<Config::Values::CFloatValue> m_colorStrength;
    SP<Config::Values::CFloatValue> m_colorWidth;
    SP<Config::Values::CColorValue> m_transmissionColor;
    SP<Config::Values::CColorValue> m_refractionColor;
    SP<Config::Values::CColorValue> m_reflectionColor;
    SP<Config::Values::CColorValue> m_highlightColor;
    SP<Config::Values::CBoolValue> m_nearest;

    std::optional<float> m_radiusOverride;
    std::optional<float> m_zoomOverride;
    std::optional<float> m_edgeWidthOverride;
};

}
