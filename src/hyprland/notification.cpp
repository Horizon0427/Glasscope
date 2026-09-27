#include "hyprland/notification.hpp"
namespace Glasscope {
void notify(HANDLE handle, const std::string& text, eIcons icon, const CHyprColor& color, std::uint64_t durationMs) {
    HyprlandAPI::addNotificationV2(
        handle, {{"text", std::string("[glasscope] ") + text}, {"time", durationMs}, {"color", color}, {"icon", icon}});
}
}
