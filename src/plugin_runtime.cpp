#define WLR_USE_UNSTABLE
#include "plugin_runtime.hpp"
#include "glasscope/core/color_probe.hpp"
#include "glasscope/core/input_router.hpp"
#include "hyprland/clipboard_writer.hpp"
#include "hyprland/coordinates.hpp"
#include "hyprland/cursor_override.hpp"
#include "hyprland/plugin_config.hpp"
#include "hyprland/render_driver.hpp"
#include <chrono>
#include <hyprland/src/event/EventBus.hpp>
#include <hyprland/src/managers/SessionLockManager.hpp>
#include <hyprland/src/managers/input/InputManager.hpp>
#include <hyprland/src/pointer/PointerManager.hpp>
#include <linux/input-event-codes.h>
namespace Glasscope {
namespace {
double nowSeconds() {
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
Vec2 cursorPosition() {
    return fromHypr(g_pInputManager->getMouseCoordsInternal());
}
bool sessionLocked() {
    return g_pSessionLockManager && g_pSessionLockManager->isSessionLocked();
}
}
struct PluginRuntime::Impl {
    friend class PluginRuntime;
    explicit Impl(HANDLE handle) : m_config(handle), m_render(handle), m_clipboard(handle) {
        m_configEnabled = m_config.enabled();
        registerListeners();
    }
    void toggle() {
        syncEnabled();
        setVisibility(!m_state.snapshot().requestedVisible);
    }
    void show() {
        syncEnabled();
        setVisibility(true);
    }
    void hide() {
        setVisibility(false);
    }
    void beginColorProbe() {
        syncEnabled();
        if (!canOperate() || m_state.pressed() || m_input.hasCapture())
            return;
        const auto before = m_state.snapshot();
        if (!m_probe.begin(m_state, cursorPosition(), nowSeconds()))
            return;
        m_cursor.update(m_probe.aiming());
        transition(before);
        damage();
    }
    void pickColor() {
        syncEnabled();
        if (!canOperate() || m_state.pressed() || (m_input.hasCapture() && m_input.owner() != PressOwner::ColorProbe))
            return;
        const auto before = m_state.snapshot();
        if (!m_probe.pick(m_state))
            return;
        m_cursor.update(m_probe.aiming());
        transition(before);
        damage();
    }
    void cancelColorProbe() {
        syncEnabled();
        if (canOperate())
            clearColorProbe();
    }
    void togglePin() {
        syncEnabled();
        if (!canOperate())
            return;
        clearColorProbe();
        const double now = nowSeconds();
        const auto before = m_state.snapshot();
        const auto cursor = cursorPosition();
        const bool pin = !before.pinned;
        m_state.setPinned(pin, pin ? before.center : cursor, now);
        transition(before);
    }
    bool isPinned() const {
        return m_state.snapshot().pinned;
    }
    float adjustZoom(float delta) {
        syncEnabled();
        const auto before = currentConfig();
        if (!canOperate())
            return before.style.zoom;
        m_config.setZoomOverride(before.style.zoom + delta);
        damageConfigTransition(before);
        return currentConfig().style.zoom;
    }
    float adjustRadius(float delta) {
        syncEnabled();
        const auto before = currentConfig();
        if (!canOperate())
            return before.style.radius;
        m_config.setRadiusOverride(before.style.radius + delta);
        damageConfigTransition(before);
        return currentConfig().style.radius;
    }
    float adjustEdgeWidth(float delta) {
        syncEnabled();
        const auto before = currentConfig();
        if (!canOperate())
            return before.style.edgeWidth;
        m_config.setEdgeWidthOverride(before.style.edgeWidth + delta);
        damageConfigTransition(before);
        return currentConfig().style.edgeWidth;
    }
    void shutdown() {
        setVisibility(false);
        m_pointerListener.reset();
        m_mouseButtonListener.reset();
        m_cursorChangedListener.reset();
        m_renderListener.reset();
        m_configReloadListener.reset();
        m_render.shutdown();
    }

  private:
    // All visibility changes enter here; cancel pending feature work before closing.
    void setVisibility(bool visible) {
        if (visible && (!configuredEnabled() || sessionLocked()))
            return;
        const auto before = m_state.snapshot();
        if (!visible)
            clearColorProbe();
        m_state.setVisible(visible, visible ? cursorPosition() : before.center, nowSeconds());
        transition(before);
    }
    bool canOperate() const {
        return configuredEnabled() && m_state.snapshot().requestedVisible && !sessionLocked();
    }
    bool configuredEnabled() const {
        return m_config.enabled() && m_render.available();
    }
    GlasscopeConfig currentConfig() const {
        auto config = m_config.snapshot();
        config.enabled = configuredEnabled();
        return config;
    }
    void transition(const LensSnapshot& before) {
        m_render.transition(before, m_state.snapshot(), currentConfig().style);
    }
    void damage() {
        m_render.damage(m_state.snapshot(), currentConfig().style);
    }
    void damageConfigTransition(const GlasscopeConfig& before) {
        const auto snapshot = m_state.snapshot();
        if (snapshot.reveal <= 0.001 && !snapshot.requestedVisible)
            return;
        m_render.damage(snapshot, before.style);
        damage();
    }
    void clearColorProbe() {
        const auto before = m_state.snapshot();
        const bool changed = m_probe.cancel(m_state, nowSeconds());
        m_clipboard.cancel();
        m_cursor.reset();
        if (changed)
            transition(before);
    }
    void syncEnabled() {
        const bool enabled = m_config.enabled();
        if (m_configEnabled && !enabled)
            disableRuntime();
        m_configEnabled = enabled;
    }
    void disableRuntime() {
        setVisibility(false);
        m_state.reset();
        // Keep input ownership until the captured release arrives.
        m_render.deferRelease();
    }
    void pointerMoved(Vec2 position, Event::SCallbackInfo& info) {
        syncEnabled();
        const auto before = m_state.snapshot();
        const auto result =
            m_input.move(m_state, position, nowSeconds(), {canOperate(), info.cancelled, sessionLocked()});
        if (result.changed)
            transition(before);
        if (result.consume)
            info.cancelled = true;
        if (result.accepted && m_probe.aiming())
            m_cursor.update(true);
    }
    void buttonChanged(IPointer::SButtonEvent event, Event::SCallbackInfo& info) {
        syncEnabled();
        const auto before = m_state.snapshot();
        const auto cursor = cursorPosition();
        const auto config = currentConfig();
        const bool down = event.state == WL_POINTER_BUTTON_STATE_PRESSED;
        const InputContext context{canOperate(), info.cancelled, sessionLocked()};
        const bool hit = down && context.enabled && !context.cancelled && !context.locked && !m_probe.aiming() &&
                         m_render.acceptsPress(cursor, before, config.style, m_probe.blocksPinnedPress());
        const auto result = m_input.button(m_state, cursor, nowSeconds(), context, event.button == BTN_LEFT, down,
                                           m_probe.aiming(), hit, config.style.radius);
        if (result.consume)
            info.cancelled = true;
        if (result.captureColor)
            pickColor();
        else if (result.changed)
            transition(before);
    }
    void onFrame() {
        syncEnabled();
        if (!m_render.beginFrame())
            return;
        const double now = nowSeconds();
        if (sessionLocked()) {
            m_input.cancelForLock(m_state);
            if (m_probe.aiming() || m_probe.capturePending())
                clearColorProbe();
            return;
        }
        const auto before = m_state.snapshot();
        if (m_probe.expire(m_state, now))
            transition(before);
        if (!configuredEnabled() || (!m_state.snapshot().requestedVisible && !m_state.rendering()))
            return;
        if (m_render.consumeFailure()) {
            setVisibility(false);
            m_render.deferRelease();
            return;
        }
        const auto target = m_render.target(m_state.snapshot());
        if (target == FrameTarget::Unsupported && (m_probe.aiming() || m_probe.capturePending()))
            clearColorProbe();
        if (target != FrameTarget::Ready)
            return;
        // Tick once, on the lens's output.
        m_state.advance(now);
        std::function<void(ColorSample)> capture;
        if (m_probe.capturePending())
            capture = [this, request = m_probe.requestId()](ColorSample sample) {
                if (!canOperate() || !m_probe.complete(m_state, request, sample, nowSeconds()))
                    return;
                m_clipboard.request(sample);
                damage();
            };
        m_render.draw(m_state.snapshot(), currentConfig().style, m_probe.visual(now),
                      m_state.needsAnimation() || m_probe.feedbackAmount(now) > 0.0F, std::move(capture));
    }
    void registerListeners() {
        m_pointerListener = Event::bus()->m_events.input.mouse.move.listen(
            [this](Vector2D position, Event::SCallbackInfo& info) { pointerMoved(fromHypr(position), info); });
        m_mouseButtonListener = Event::bus()->m_events.input.mouse.button.listen(
            [this](IPointer::SButtonEvent event, Event::SCallbackInfo& info) { buttonChanged(event, info); });
        m_cursorChangedListener = Pointer::mgr()->m_events.cursorChanged.listen([this] {
            if (m_probe.aiming())
                m_cursor.update(true);
        });
        m_renderListener = Event::bus()->m_events.render.stage.listen([this](eRenderStage stage) {
            if (stage == RENDER_LAST_MOMENT)
                onFrame();
        });
        m_configReloadListener = Event::bus()->m_events.config.reloaded.listen([this] {
            const auto before = currentConfig();
            m_config.clearOverrides();
            syncEnabled();
            if (m_configEnabled)
                damageConfigTransition(before);
        });
    }
    PluginConfig m_config;
    LensState m_state;
    ColorProbe m_probe;
    InputRouter m_input;
    RenderDriver m_render;
    ClipboardWriter m_clipboard;
    CursorOverride m_cursor;
    bool m_configEnabled = false;
    CHyprSignalListener m_pointerListener;
    CHyprSignalListener m_mouseButtonListener;
    CHyprSignalListener m_cursorChangedListener;
    CHyprSignalListener m_renderListener;
    CHyprSignalListener m_configReloadListener;
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

void PluginRuntime::togglePin() {
    m_impl->togglePin();
}

bool PluginRuntime::isPinned() const {
    return m_impl->isPinned();
}

void PluginRuntime::beginColorProbe() {
    m_impl->beginColorProbe();
}

void PluginRuntime::pickColor() {
    m_impl->pickColor();
}

void PluginRuntime::cancelColorProbe() {
    m_impl->cancelColorProbe();
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

#ifdef GLASSCOPE_NATIVE_TESTING
std::pair<GlasscopeConfig, LensSnapshot> PluginRuntime::testSnapshot() const {
    return {m_impl->currentConfig(), m_impl->m_state.snapshot()};
}
#endif

}
