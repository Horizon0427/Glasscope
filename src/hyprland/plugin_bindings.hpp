#pragma once
#include <hyprland/src/plugins/PluginAPI.hpp>
namespace Glasscope {
class PluginRuntime;
void registerBindings(HANDLE handle);
void unregisterBindings(HANDLE handle);
void bindRuntime(Glasscope::PluginRuntime* runtime);

}
