#define WLR_USE_UNSTABLE
#include "hyprland/render_driver.hpp"
#include "glasscope/geometry/lens_geometry.hpp"
#include "hyprland/coordinates.hpp"
#include "hyprland/lens_pass_element.hpp"
#include "hyprland/notification.hpp"
#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/state/MonitorState.hpp>
namespace Glasscope {
namespace {
CBox lensBounds(Vec2 center, const std::array<Vec2, 3>& trail, const LensStyle& style) {
    const double extent = lensExtent(style, style.radius, style.edgeWidth, trail);
    return {center.x - extent, center.y - extent, extent * 2.0, extent * 2.0};
}
}
bool RenderDriver::beginFrame() {
    if (!g_pHyprRenderer->m_renderData.pMonitor.lock())
        return false;
    if (m_releaseRequested) {
        m_renderer.destroy();
        m_releaseRequested = false;
    }
    return true;
}
FrameTarget RenderDriver::target(const LensSnapshot& snapshot) {
    const auto monitor = g_pHyprRenderer->m_renderData.pMonitor.lock();
    const auto owner = State::monitorState()->query().vec(toHypr(snapshot.center)).run();
    if (!owner || owner != monitor)
        return FrameTarget::OtherOutput;
    if (monitor->m_transform != WL_OUTPUT_TRANSFORM_NORMAL) {
        if (!m_transformWarningShown) {
            notify(m_handle, "Glasscope currently supports unrotated outputs only", ICON_WARNING,
                   CHyprColor{1.0F, 0.72F, 0.25F, 1.0F});
            m_transformWarningShown = true;
        }
        return FrameTarget::Unsupported;
    }
    return FrameTarget::Ready;
}
bool RenderDriver::consumeFailure() {
    if (!m_renderer.failed() || m_disabled)
        return false;
    m_disabled = true;
    notify(m_handle, m_renderer.takeError().value_or("Glasscope renderer failed"), ICON_ERROR,
           CHyprColor{1.0F, 0.25F, 0.25F, 1.0F});
    return true;
}
bool RenderDriver::acceptsPress(Vec2 cursor, const LensSnapshot& snapshot, const LensStyle& style,
                                bool probeBlocked) const {
    if (!snapshot.pinned || !snapshot.requestedVisible || probeBlocked)
        return false;
    const auto monitor = State::monitorState()->query().vec(toHypr(snapshot.center)).run();
    if (!monitor || monitor->m_transform != WL_OUTPUT_TRANSFORM_NORMAL)
        return false;
    const auto pointerMonitor = State::monitorState()->query().vec(toHypr(cursor)).run();
    return pointerMonitor == monitor && lensContains(cursor, snapshot, style, monitor->m_scale);
}
void RenderDriver::damage(const LensSnapshot& snapshot, const LensStyle& style) const {
    if (g_pHyprRenderer)
        g_pHyprRenderer->damageBox(lensBounds(snapshot.center, snapshot.trailNodes, style));
}
void RenderDriver::transition(const LensSnapshot& before, const LensSnapshot& after, const LensStyle& style) const {
    if (before.reveal > 0.001 || before.requestedVisible)
        damage(before, style);
    if (after.reveal > 0.001 || after.requestedVisible)
        damage(after, style);
}
void RenderDriver::deferRelease() {
    if (g_pHyprRenderer)
        g_pHyprRenderer->currentPass().removeAllOfType("CGlasscopeLensPassElement");
    m_releaseRequested = true;
}
void RenderDriver::shutdown() {
    if (g_pHyprRenderer)
        g_pHyprRenderer->currentPass().removeAllOfType("CGlasscopeLensPassElement");
    if (Render::GL::g_pHyprOpenGL) {
        Render::GL::g_pHyprOpenGL->makeEGLCurrent();
        m_renderer.destroy();
    }
}
void RenderDriver::draw(const LensSnapshot& snapshot, const LensStyle& style, const ProbeVisual& probe, bool animate,
                        std::function<void(ColorSample)> onCapture) {
    if (animate)
        damage(snapshot, style);
    if (snapshot.reveal <= 0.001) {
        m_renderer.releaseCopyTexture();
        return;
    }
    const auto monitor = g_pHyprRenderer->m_renderData.pMonitor.lock();
    if (!monitor)
        return;
    const Vector2D centerGlobal = toHypr(snapshot.center);
    const Vector2D centerLocal = centerGlobal - monitor->m_position;
    CBox physicalLensBounds = lensBounds(snapshot.center, snapshot.trailNodes, style);
    physicalLensBounds.translate(-monitor->m_position).scale(monitor->m_scale).round();

    CRegion intersectingDamage = g_pHyprRenderer->m_renderData.damage.copy();
    intersectingDamage.intersect(physicalLensBounds);
    if (intersectingDamage.empty())
        return;

    g_pHyprRenderer->m_renderData.damage.add(physicalLensBounds);
    monitor->m_damage.damage(physicalLensBounds);

    g_pHyprRenderer->addPassElement(
        makeUnique<LensPassElement>(m_renderer, LensPassData{
                                                    .monitor = monitor,
                                                    .centerLocal = centerLocal,
                                                    .velocity = toHypr(snapshot.velocity),
                                                    .pullAxis = toHypr(snapshot.pullAxis),
                                                    .trailNodes =
                                                        {
                                                            toHypr(snapshot.trailNodes[0]),
                                                            toHypr(snapshot.trailNodes[1]),
                                                            toHypr(snapshot.trailNodes[2]),
                                                        },
                                                    .style = style,
                                                    .reveal = static_cast<float>(snapshot.reveal),
                                                    .wobble = static_cast<float>(snapshot.wobble),
                                                    .interactionWobble = static_cast<float>(snapshot.interactionWobble),
                                                    .pinned = snapshot.pinned,
                                                    .pullShare = static_cast<float>(snapshot.pullShare),
                                                    .timeSeconds = static_cast<float>(snapshot.phase),
                                                    .colorProbeAmount = probe.amount,
                                                    .colorProbeCaptured = probe.captured,
                                                    .colorProbeColor = probe.color,
                                                    .captureColor = probe.capture,
                                                    .onColorCaptured = std::move(onCapture),
                                                }));
}

}
