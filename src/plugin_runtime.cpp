#define WLR_USE_UNSTABLE

#include "glasscope/lens_config.hpp"
#include "glasscope/lens_geometry.hpp"
#include "glasscope/lens_pass_element.hpp"
#include "glasscope/lens_renderer.hpp"
#include "glasscope/lens_state.hpp"

#include "plugin_config.hpp"
#include "plugin_runtime.hpp"

#include <hyprland/src/event/EventBus.hpp>
#include <hyprland/src/managers/SessionLockManager.hpp>
#include <hyprland/src/managers/input/InputManager.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/state/MonitorState.hpp>

#include <algorithm>
#include <any>
#include <chrono>
#include <cmath>
#include <memory>
#include <string>

namespace Glasscope {
namespace {

double nowSeconds() {
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration<double>(now).count();
}

Vec2 fromHypr(Vector2D value) {
    return {value.x, value.y};
}

Vector2D toHypr(Vec2 value) {
    return {value.x, value.y};
}

CBox lensBounds(Vec2 center, const std::array<Vec2, 3>& trailNodes, const LensStyle& style) {
    double maximumTrailLength = 0.0;
    for (const Vec2 node : trailNodes)
        maximumTrailLength = std::max(maximumTrailLength, std::hypot(node.x, node.y));
    const double extent = lensExtent({
        .radius = style.radius,
        .edgeWidth = style.edgeWidth,
        .edgeStrength = style.edgeStrength,
        .refraction = style.refraction,
        .dispersion = style.dispersion,
        .motionStrength = style.motionStrength,
        .maximumTrailLength = maximumTrailLength,
    });
    return {
        center.x - extent,
        center.y - extent,
        extent * 2.0,
        extent * 2.0,
    };
}

} // namespace

struct PluginRuntime::Impl {
  public:
    explicit Impl(HANDLE handle) : m_handle(handle), m_config(handle) {
        m_configEnabled = m_config.enabled();
        registerListeners();
    }

    ~Impl() = default;

    void toggle() {
        if (!configuredEnabled())
            return;
        const Vector2D cursor = g_pInputManager->getMouseCoordsInternal();
        const LensSnapshot before = m_state.snapshot();
        m_state.toggle(fromHypr(cursor), nowSeconds());
        damageTransition(before, m_state.snapshot());
    }

    void show() {
        if (!configuredEnabled())
            return;
        const Vector2D cursor = g_pInputManager->getMouseCoordsInternal();
        const LensSnapshot before = m_state.snapshot();
        m_state.setVisible(true, fromHypr(cursor), nowSeconds());
        damageTransition(before, m_state.snapshot());
    }

    void hide() {
        const LensSnapshot before = m_state.snapshot();
        m_state.setVisible(false, before.center, nowSeconds());
        damageTransition(before, m_state.snapshot());
    }

    float adjustZoom(float delta) {
        const GlasscopeConfig before = currentConfig();
        if (!before.enabled)
            return before.style.zoom;
        m_config.setZoomOverride(before.style.zoom + delta);
        damageConfigTransition(before);
        return currentConfig().style.zoom;
    }

    float adjustRadius(float delta) {
        const GlasscopeConfig before = currentConfig();
        if (!before.enabled)
            return before.style.radius;
        m_config.setRadiusOverride(before.style.radius + delta);
        damageConfigTransition(before);
        return currentConfig().style.radius;
    }

    float adjustEdgeWidth(float delta) {
        const GlasscopeConfig before = currentConfig();
        if (!before.enabled)
            return before.style.edgeWidth;
        m_config.setEdgeWidthOverride(before.style.edgeWidth + delta);
        damageConfigTransition(before);
        return currentConfig().style.edgeWidth;
    }

    void shutdown() {
        m_pointerListener.reset();
        m_renderListener.reset();
        m_configReloadListener.reset();

        if (g_pHyprRenderer)
            g_pHyprRenderer->currentPass().removeAllOfType("CGlasscopeLensPassElement");

        if (Render::GL::g_pHyprOpenGL) {
            Render::GL::g_pHyprOpenGL->makeEGLCurrent();
            m_renderer.destroy();
        }
    }

  private:
    void registerListeners() {
        m_pointerListener =
            Event::bus()->m_events.input.mouse.move.listen([this](Vector2D position, Event::SCallbackInfo&) {
                if (!configuredEnabled())
                    return;
                const double now = nowSeconds();
                const LensSnapshot before = m_state.snapshot();
                if (before.requestedVisible || m_state.rendering()) {
                    m_state.move(fromHypr(position), now);
                    damageTransition(before, m_state.snapshot());
                }
            });

        m_renderListener = Event::bus()->m_events.render.stage.listen([this](eRenderStage stage) {
            if (stage == RENDER_LAST_MOMENT)
                onLastRenderMoment();
        });

        m_configReloadListener = Event::bus()->m_events.config.reloaded.listen([this] {
            const GlasscopeConfig before = currentConfig();
            const bool enabled = m_config.enabled();
            const bool hadOverrides = m_config.hasOverrides();
            m_config.clearOverrides();

            if (m_configEnabled && !enabled)
                disableRuntime();
            m_configEnabled = enabled;

            if (enabled && hadOverrides)
                damageConfigTransition(before);
        });
    }

    void onLastRenderMoment() {
        const auto monitor = g_pHyprRenderer->m_renderData.pMonitor.lock();
        if (!monitor)
            return;

        if (m_releaseRendererRequested) {
            m_renderer.destroy();
            m_releaseRendererRequested = false;
        }

        if (!configuredEnabled() || (!m_state.snapshot().requestedVisible && !m_state.rendering()))
            return;

        if (m_renderer.failed()) {
            handleRendererFailure();
            return;
        }
        LensSnapshot snapshot = m_state.snapshot();
        const auto cursorMonitor = State::monitorState()->query().vec(toHypr(snapshot.center)).run();
        if (!cursorMonitor || cursorMonitor != monitor)
            return;

        if (g_pSessionLockManager && g_pSessionLockManager->isSessionLocked())
            return;

        if (monitor->m_transform != WL_OUTPUT_TRANSFORM_NORMAL) {
            if (!m_transformWarningShown) {
                notify("Glasscope currently supports unrotated outputs only", ICON_WARNING,
                       CHyprColor{1.0F, 0.72F, 0.25F, 1.0F});
                m_transformWarningShown = true;
            }
            return;
        }

        const double now = nowSeconds();
        m_state.advance(now);
        snapshot = m_state.snapshot();

        if (m_state.needsAnimation())
            damage(snapshot.center, snapshot.trailNodes);
        if (!m_state.rendering()) {
            m_renderer.releaseCopyTexture();
            return;
        }

        const GlasscopeConfig config = currentConfig();
        const Vector2D centerGlobal = toHypr(snapshot.center);
        const Vector2D centerLocal = centerGlobal - monitor->m_position;
        CBox physicalLensBounds = lensBounds(snapshot.center, snapshot.trailNodes, config.style);
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
                                                        .trailNodes =
                                                            {
                                                                toHypr(snapshot.trailNodes[0]),
                                                                toHypr(snapshot.trailNodes[1]),
                                                                toHypr(snapshot.trailNodes[2]),
                                                            },
                                                        .style = config.style,
                                                        .reveal = static_cast<float>(snapshot.reveal),
                                                        .wobble = static_cast<float>(snapshot.wobble),
                                                        .timeSeconds = static_cast<float>(snapshot.phase),
                                                    }));
    }

    GlasscopeConfig currentConfig() const {
        GlasscopeConfig config = m_config.snapshot();
        config.enabled = configuredEnabled();
        return config;
    }

    bool configuredEnabled() const {
        return m_config.enabled() && !m_rendererDisabled;
    }

    void disableRuntime() {
        const LensSnapshot before = m_state.snapshot();
        damage(before.center, before.trailNodes);
        m_state.reset();

        if (g_pHyprRenderer)
            g_pHyprRenderer->currentPass().removeAllOfType("CGlasscopeLensPassElement");
        m_releaseRendererRequested = true;
    }

    void damageTransition(const LensSnapshot& before, const LensSnapshot& after) {
        if (before.reveal > 0.001 || before.requestedVisible)
            damage(before.center, before.trailNodes);
        if (after.reveal > 0.001 || after.requestedVisible)
            damage(after.center, after.trailNodes);
    }

    void damageConfigTransition(const GlasscopeConfig& before) const {
        const LensSnapshot snapshot = m_state.snapshot();
        if (snapshot.reveal <= 0.001 && !snapshot.requestedVisible)
            return;

        damage(snapshot.center, snapshot.trailNodes, before);
        damage(snapshot.center, snapshot.trailNodes, currentConfig());
    }

    void damage(Vec2 center, const std::array<Vec2, 3>& trailNodes = {}) const {
        damage(center, trailNodes, currentConfig());
    }

    void damage(Vec2 center, const std::array<Vec2, 3>& trailNodes, const GlasscopeConfig& config) const {
        if (!g_pHyprRenderer)
            return;
        g_pHyprRenderer->damageBox(lensBounds(center, trailNodes, config.style));
    }

    void handleRendererFailure() {
        const auto error = m_renderer.takeError();
        m_rendererDisabled = true;
        if (!m_rendererFailureShown) {
            m_rendererFailureShown = true;
            notify(error.value_or("Glasscope renderer failed"), ICON_ERROR, CHyprColor{1.0F, 0.25F, 0.25F, 1.0F});
        }
        const LensSnapshot snapshot = m_state.snapshot();
        m_state.setVisible(false, snapshot.center, nowSeconds());
        damage(snapshot.center, snapshot.trailNodes);
        if (g_pHyprRenderer)
            g_pHyprRenderer->currentPass().removeAllOfType("CGlasscopeLensPassElement");
        m_releaseRendererRequested = true;
    }

    void notify(const std::string& text, eIcons icon, const CHyprColor& color) const {
        HyprlandAPI::addNotificationV2(m_handle, {
                                                     {"text", std::string("[glasscope] ") + text},
                                                     {"time", static_cast<uint64_t>(5000)},
                                                     {"color", color},
                                                     {"icon", icon},
                                                 });
    }

    HANDLE m_handle = nullptr;
    PluginConfig m_config;
    LensState m_state;
    LensRenderer m_renderer;

    CHyprSignalListener m_pointerListener;
    CHyprSignalListener m_renderListener;
    CHyprSignalListener m_configReloadListener;
    bool m_rendererFailureShown = false;
    bool m_rendererDisabled = false;
    bool m_transformWarningShown = false;
    bool m_configEnabled = false;
    bool m_releaseRendererRequested = false;
};

PluginRuntime::PluginRuntime(HANDLE handle) : m_impl(std::make_unique<Impl>(handle)) {}

PluginRuntime::~PluginRuntime() = default;

void PluginRuntime::toggle() {
    m_impl->toggle();
}

void PluginRuntime::show() {
    m_impl->show();
}

void PluginRuntime::hide() {
    m_impl->hide();
}

float PluginRuntime::adjustZoom(float delta) {
    return m_impl->adjustZoom(delta);
}

float PluginRuntime::adjustRadius(float delta) {
    return m_impl->adjustRadius(delta);
}

float PluginRuntime::adjustEdgeWidth(float delta) {
    return m_impl->adjustEdgeWidth(delta);
}

void PluginRuntime::shutdown() {
    m_impl->shutdown();
}

} // namespace Glasscope
