#pragma once
#include <cstdint>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <string>
namespace Glasscope {
void notify(HANDLE handle, const std::string& text, eIcons icon, const CHyprColor& color,
            std::uint64_t durationMs = 5000);
}
