#include "hyprland/plugin_bindings.hpp"
#include "plugin_runtime.hpp"
#include <cmath>
#include <optional>
#include <stdexcept>
extern "C" {
#include <lua.h>
}
namespace Glasscope {
namespace {
Glasscope::PluginRuntime* g_plugin = nullptr;
int luaToggle(lua_State*) {
    if (g_plugin)
        g_plugin->toggle();
    return 0;
}

int luaShow(lua_State*) {
    if (g_plugin)
        g_plugin->show();
    return 0;
}

int luaHide(lua_State*) {
    if (g_plugin)
        g_plugin->hide();
    return 0;
}

int luaTogglePin(lua_State*) {
    if (g_plugin)
        g_plugin->togglePin();
    return 0;
}

int luaIsPinned(lua_State* state) {
    lua_pushboolean(state, g_plugin && g_plugin->isPinned());
    return 1;
}

int luaBeginColorProbe(lua_State*) {
    if (g_plugin)
        g_plugin->beginColorProbe();
    return 0;
}

int luaPickColor(lua_State*) {
    if (g_plugin)
        g_plugin->pickColor();
    return 0;
}

int luaCancelColorProbe(lua_State*) {
    if (g_plugin)
        g_plugin->cancelColorProbe();
    return 0;
}

std::optional<float> luaDelta(lua_State* state) {
    if (lua_gettop(state) < 1 || lua_isnumber(state, 1) == 0)
        return std::nullopt;
    const float value = static_cast<float>(lua_tonumber(state, 1));
    if (!std::isfinite(value))
        return std::nullopt;
    return value;
}

int luaAdjustZoom(lua_State* state) {
    const auto delta = luaDelta(state);
    if (!g_plugin || !delta)
        return 0;
    lua_pushnumber(state, g_plugin->adjustZoom(*delta));
    return 1;
}

int luaAdjustRadius(lua_State* state) {
    const auto delta = luaDelta(state);
    if (!g_plugin || !delta)
        return 0;
    lua_pushnumber(state, g_plugin->adjustRadius(*delta));
    return 1;
}

int luaAdjustEdgeWidth(lua_State* state) {
    const auto delta = luaDelta(state);
    if (!g_plugin || !delta)
        return 0;
    lua_pushnumber(state, g_plugin->adjustEdgeWidth(*delta));
    return 1;
}

}
void bindRuntime(Glasscope::PluginRuntime* runtime) {
    g_plugin = runtime;
}
void unregisterBindings(HANDLE handle) {
    if (!handle)
        return;
    HyprlandAPI::removeDispatcher(handle, "glasscope:toggle");
    HyprlandAPI::removeDispatcher(handle, "glasscope:show");
    HyprlandAPI::removeDispatcher(handle, "glasscope:hide");
    HyprlandAPI::removeDispatcher(handle, "glasscope:toggle-pin");
    HyprlandAPI::removeDispatcher(handle, "glasscope:begin-color-probe");
    HyprlandAPI::removeDispatcher(handle, "glasscope:pick-color");
    HyprlandAPI::removeDispatcher(handle, "glasscope:cancel-color-probe");
    HyprlandAPI::removeLuaFunction(handle, "glasscope", "toggle");
    HyprlandAPI::removeLuaFunction(handle, "glasscope", "show");
    HyprlandAPI::removeLuaFunction(handle, "glasscope", "hide");
    HyprlandAPI::removeLuaFunction(handle, "glasscope", "toggle_pin");
    HyprlandAPI::removeLuaFunction(handle, "glasscope", "is_pinned");
    HyprlandAPI::removeLuaFunction(handle, "glasscope", "begin_color_probe");
    HyprlandAPI::removeLuaFunction(handle, "glasscope", "pick_color");
    HyprlandAPI::removeLuaFunction(handle, "glasscope", "cancel_color_probe");
    HyprlandAPI::removeLuaFunction(handle, "glasscope", "adjust_zoom");
    HyprlandAPI::removeLuaFunction(handle, "glasscope", "adjust_radius");
    HyprlandAPI::removeLuaFunction(handle, "glasscope", "adjust_edge_width");
}

void registerBindings(HANDLE handle) {
    const auto addDispatcher = [handle](const std::string& name, auto callback) {
        if (!HyprlandAPI::addDispatcherV2(handle, name, callback))
            throw std::runtime_error("glasscope: failed to register dispatcher " + name);
    };

    addDispatcher("glasscope:toggle", [](const std::string&) {
        if (g_plugin)
            g_plugin->toggle();
        return SDispatchResult{};
    });
    addDispatcher("glasscope:show", [](const std::string&) {
        if (g_plugin)
            g_plugin->show();
        return SDispatchResult{};
    });
    addDispatcher("glasscope:hide", [](const std::string&) {
        if (g_plugin)
            g_plugin->hide();
        return SDispatchResult{};
    });
    addDispatcher("glasscope:begin-color-probe", [](const std::string&) {
        if (g_plugin)
            g_plugin->beginColorProbe();
        return SDispatchResult{};
    });

    addDispatcher("glasscope:toggle-pin", [](const std::string&) {
        if (g_plugin)
            g_plugin->togglePin();
        return SDispatchResult{};
    });
    addDispatcher("glasscope:pick-color", [](const std::string&) {
        if (g_plugin)
            g_plugin->pickColor();
        return SDispatchResult{};
    });
    addDispatcher("glasscope:cancel-color-probe", [](const std::string&) {
        if (g_plugin)
            g_plugin->cancelColorProbe();
        return SDispatchResult{};
    });

    if (!HyprlandAPI::addLuaFunction(handle, "glasscope", "toggle", luaToggle) ||
        !HyprlandAPI::addLuaFunction(handle, "glasscope", "show", luaShow) ||
        !HyprlandAPI::addLuaFunction(handle, "glasscope", "hide", luaHide) ||
        !HyprlandAPI::addLuaFunction(handle, "glasscope", "toggle_pin", luaTogglePin) ||
        !HyprlandAPI::addLuaFunction(handle, "glasscope", "is_pinned", luaIsPinned) ||
        !HyprlandAPI::addLuaFunction(handle, "glasscope", "begin_color_probe", luaBeginColorProbe) ||
        !HyprlandAPI::addLuaFunction(handle, "glasscope", "pick_color", luaPickColor) ||
        !HyprlandAPI::addLuaFunction(handle, "glasscope", "cancel_color_probe", luaCancelColorProbe) ||
        !HyprlandAPI::addLuaFunction(handle, "glasscope", "adjust_zoom", luaAdjustZoom) ||
        !HyprlandAPI::addLuaFunction(handle, "glasscope", "adjust_radius", luaAdjustRadius) ||
        !HyprlandAPI::addLuaFunction(handle, "glasscope", "adjust_edge_width", luaAdjustEdgeWidth))
        throw std::runtime_error("glasscope: failed to register Lua functions");
}

}
