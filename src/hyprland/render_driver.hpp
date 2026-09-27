#pragma once
#include "glasscope/core/color_probe.hpp"
#include "glasscope/model/lens_config.hpp"
#include "glasscope/model/lens_snapshot.hpp"
#include "render/lens_renderer.hpp"
#include <functional>
#include <hyprland/src/plugins/PluginAPI.hpp>
namespace Glasscope {
enum class FrameTarget { OtherOutput, Unsupported, Ready };
class RenderDriver {
  public:
    explicit RenderDriver(HANDLE handle) : m_handle(handle) {}
    bool beginFrame();
    FrameTarget target(const LensSnapshot& snapshot);
    bool consumeFailure();
    [[nodiscard]] bool available() const {
        return !m_disabled;
    }
    bool acceptsPress(Vec2 cursor, const LensSnapshot& snapshot, const LensStyle& style, bool probeBlocked) const;
    void draw(const LensSnapshot& snapshot, const LensStyle& style, const ProbeVisual& probe, bool animate,
              std::function<void(ColorSample)> onCapture);
    void damage(const LensSnapshot& snapshot, const LensStyle& style) const;
    void transition(const LensSnapshot& before, const LensSnapshot& after, const LensStyle& style) const;
    void deferRelease();
    void shutdown();

  private:
    HANDLE m_handle;
    LensRenderer m_renderer;
    bool m_disabled = false;
    bool m_transformWarningShown = false;
    bool m_releaseRequested = false;
};
}
