#pragma once

#include <hyprland/src/plugins/PluginAPI.hpp>

#include <memory>
#ifdef GLASSCOPE_NATIVE_TESTING
#include "glasscope/model/lens_config.hpp"
#include "glasscope/model/lens_snapshot.hpp"
#include <utility>
#endif

namespace Glasscope {

class PluginRuntime {
  public:
    explicit PluginRuntime(HANDLE handle);
    ~PluginRuntime();

    PluginRuntime(const PluginRuntime&) = delete;
    PluginRuntime& operator=(const PluginRuntime&) = delete;

    void toggle();
    void show();
    void hide();
    void togglePin();
    [[nodiscard]] bool isPinned() const;
    void beginColorProbe();
    void pickColor();
    void cancelColorProbe();
    float adjustZoom(float delta);
    float adjustRadius(float delta);
    float adjustEdgeWidth(float delta);
    void shutdown();
#ifdef GLASSCOPE_NATIVE_TESTING
    [[nodiscard]] std::pair<GlasscopeConfig, LensSnapshot> testSnapshot() const;
#endif

  private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}
