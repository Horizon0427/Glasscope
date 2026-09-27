#include "hyprland/cursor_override.hpp"
#include <hyprland/src/render/Renderer.hpp>
namespace Glasscope {
void CursorOverride::update(bool aiming) {
    if (!aiming) {
        reset();
        return;
    }
    if (!g_pHyprRenderer)
        return;
    m_hidden = true;
    g_pHyprRenderer->setCursorHidden(true);
}
void CursorOverride::reset() {
    if (!m_hidden)
        return;
    m_hidden = false;
    if (g_pHyprRenderer)
        g_pHyprRenderer->ensureCursorRenderingMode();
}
}
