#pragma once
#include "glasscope/model/color_sample.hpp"
#include <hyprland/src/managers/eventLoop/EventLoopManager.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
namespace Glasscope {
class ClipboardWriter {
  public:
    explicit ClipboardWriter(HANDLE handle) : m_handle(handle) {}
    void request(ColorSample sample);
    void cancel();

  private:
    HANDLE m_handle;
    UP<SEventLoopDoLaterLock> m_pending;
};
}
