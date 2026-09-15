#pragma once

#include <hyprland/src/plugins/PluginAPI.hpp>

#include <memory>

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
    void beginColorProbe();
    void pickColor();
    void cancelColorProbe();
    float adjustZoom(float delta);
    float adjustRadius(float delta);
    float adjustEdgeWidth(float delta);
    void shutdown();

  private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}
