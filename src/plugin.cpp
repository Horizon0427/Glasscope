#define WLR_USE_UNSTABLE

#include "hyprland/plugin_bindings.hpp"
#include "plugin_runtime.hpp"
#ifdef GLASSCOPE_NATIVE_TESTING
#include "observer.hpp"
#endif

#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/render/Renderer.hpp>

#include <memory>
#include <stdexcept>
#include <string>

namespace {

std::unique_ptr<Glasscope::PluginRuntime> g_plugin;
HANDLE g_handle = nullptr;

void notifyInitFailure(const std::string& text) {
    if (!g_handle)
        return;
    HyprlandAPI::addNotificationV2(g_handle, {
                                                 {"text", std::string("[glasscope] ") + text},
                                                 {"time", static_cast<uint64_t>(6000)},
                                                 {"color", CHyprColor{1.0F, 0.2F, 0.2F, 1.0F}},
                                                 {"icon", ICON_ERROR},
                                             });
}

}

APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    g_handle = handle;

    const std::string serverHash = __hyprland_api_get_hash();
    const std::string clientHash = __hyprland_api_get_client_hash();
    if (serverHash != clientHash) {
        notifyInitFailure("Hyprland header/runtime version mismatch");
        throw std::runtime_error("glasscope: Hyprland ABI mismatch");
    }
    if (!g_pHyprRenderer || g_pHyprRenderer->type() != Render::IHyprRenderer::RT_GL) {
        notifyInitFailure("the current renderer is not the OpenGL backend");
        throw std::runtime_error("glasscope: OpenGL renderer required");
    }

    try {
        Glasscope::registerBindings(handle);
        g_plugin = std::make_unique<Glasscope::PluginRuntime>(handle);
        Glasscope::bindRuntime(g_plugin.get());
#ifdef GLASSCOPE_NATIVE_TESTING
        registerNativeObserver(handle, *g_plugin);
#endif
    } catch (...) {
#ifdef GLASSCOPE_NATIVE_TESTING
        unregisterNativeObserver(handle);
#endif
        Glasscope::bindRuntime(nullptr);
        Glasscope::unregisterBindings(handle);
        if (g_plugin)
            g_plugin->shutdown();
        g_plugin.reset();
        notifyInitFailure("initialization failed");
        throw;
    }

    HyprlandAPI::addNotificationV2(handle, {
                                               {"text", std::string("[glasscope] Ready")},
                                               {"time", static_cast<uint64_t>(2500)},
                                               {"color", CHyprColor{0.55F, 0.89F, 0.83F, 1.0F}},
                                               {"icon", ICON_OK},
                                           });
    return {
        "glasscope",
        "An interactive liquid-glass magnifier for Hyprland",
        "horizon",
        "0.7.0",
    };
}

APICALL EXPORT void PLUGIN_EXIT() {
#ifdef GLASSCOPE_NATIVE_TESTING
    unregisterNativeObserver(g_handle);
#endif
    Glasscope::unregisterBindings(g_handle);
    Glasscope::bindRuntime(nullptr);
    if (g_plugin)
        g_plugin->shutdown();
    g_plugin.reset();
    g_handle = nullptr;
}
