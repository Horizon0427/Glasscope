#include "glasscope/lens_pass_element.hpp"

#include "glasscope/lens_geometry.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace Glasscope {

LensPassElement::LensPassElement(LensRenderer& renderer, LensPassData data)
    : m_renderer(renderer), m_data(std::move(data)) {}

std::vector<UP<IPassElement>> LensPassElement::draw() {
    const auto monitor = m_data.monitor.lock();
    if (!monitor || monitor->m_transform != WL_OUTPUT_TRANSFORM_NORMAL)
        return {};

    const float scale = monitor->m_scale;
    const auto snapshotWidth = static_cast<int>(std::lround(monitor->m_pixelSize.x));
    const auto snapshotHeight = static_cast<int>(std::lround(monitor->m_pixelSize.y));
    const float speedScale = 1.0F / 1800.0F;
    const std::array<float, 6> trailNodes = {
        static_cast<float>(m_data.trailNodes[0].x) * scale, static_cast<float>(m_data.trailNodes[0].y) * scale,
        static_cast<float>(m_data.trailNodes[1].x) * scale, static_cast<float>(m_data.trailNodes[1].y) * scale,
        static_cast<float>(m_data.trailNodes[2].x) * scale, static_cast<float>(m_data.trailNodes[2].y) * scale,
    };

    const bool drawn = m_renderer.draw({
        .framebufferWidth = snapshotWidth,
        .framebufferHeight = snapshotHeight,
        .centerX = static_cast<float>(m_data.centerLocal.x) * scale,
        .centerY = static_cast<float>(m_data.centerLocal.y) * scale,
        .scale = scale,
        .style = m_data.style,
        .timeSeconds = m_data.timeSeconds,
        .velocityX = std::clamp(static_cast<float>(m_data.velocity.x) * speedScale, -1.0F, 1.0F),
        .velocityY = std::clamp(static_cast<float>(m_data.velocity.y) * speedScale, -1.0F, 1.0F),
        .pullAxis = {static_cast<float>(m_data.pullAxis.x), static_cast<float>(m_data.pullAxis.y)},
        .trailNodes = trailNodes,
        .reveal = m_data.reveal,
        .wobble = m_data.wobble,
        .interactionWobble = m_data.interactionWobble,
        .pinned = m_data.pinned,
        .pullShare = m_data.pullShare,
        .colorProbeAmount = m_data.colorProbeAmount,
        .colorProbeCaptured = m_data.colorProbeCaptured,
        .colorProbeColor = m_data.colorProbeColor,
        .captureColor = m_data.captureColor,
    });
    if (const auto sample = m_renderer.takeColorSample(); drawn && sample && m_data.onColorCaptured)
        m_data.onColorCaptured(*sample);
    return {};
}

bool LensPassElement::needsLiveBlur() {
    return false;
}

bool LensPassElement::needsPrecomputeBlur() {
    return false;
}

std::optional<CBox> LensPassElement::boundingBox() {
    double maximumTrailLength = 0.0;
    for (const Vector2D node : m_data.trailNodes)
        maximumTrailLength = std::max(maximumTrailLength, std::hypot(node.x, node.y));
    const double extent = lensExtent({
        .radius = m_data.style.radius,
        .edgeWidth = m_data.style.edgeWidth,
        .edgeStrength = m_data.style.edgeStrength,
        .refraction = m_data.style.refraction,
        .dispersion = m_data.style.dispersion,
        .motionStrength = m_data.style.motionStrength,
        .maximumTrailLength = maximumTrailLength,
    });
    return CBox{
        m_data.centerLocal.x - extent,
        m_data.centerLocal.y - extent,
        extent * 2.0,
        extent * 2.0,
    };
}

CRegion LensPassElement::opaqueRegion() {
    return {};
}

const char* LensPassElement::passName() {
    return "CGlasscopeLensPassElement";
}

ePassElementType LensPassElement::type() {
    return EK_CUSTOM;
}

}
