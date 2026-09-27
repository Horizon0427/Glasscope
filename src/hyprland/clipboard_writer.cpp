#include "hyprland/clipboard_writer.hpp"
#include "hyprland/notification.hpp"
#include <hyprland/src/config/supplementary/executor/Executor.hpp>
#include <unistd.h>
namespace Glasscope {
namespace {
std::string rgbText(ColorSample sample) {
    return "rgb(" + std::to_string(sample.red) + ", " + std::to_string(sample.green) + ", " +
           std::to_string(sample.blue) + ")";
}

bool copyRgbToClipboard(const std::string& text) {
    constexpr const char* WL_COPY = "/usr/bin/wl-copy";
    if (::access(WL_COPY, X_OK) != 0 || !Config::Supplementary::executor())
        return false;

    const std::string command = std::string(WL_COPY) + " --type 'text/plain;charset=utf-8' -- '" + text + "'";
    return Config::Supplementary::executor()->spawnRaw(command).has_value();
}

}
void ClipboardWriter::request(ColorSample sample) {
    // Deferred work must not capture runtime objects.
    m_pending = g_pEventLoopManager->doLaterLock([handle = m_handle, sample] {
        const std::string text = rgbText(sample);
        const bool launched = copyRgbToClipboard(text);
        const CHyprColor color{static_cast<float>(sample.red) / 255.0F, static_cast<float>(sample.green) / 255.0F,
                               static_cast<float>(sample.blue) / 255.0F, 1.0F};
        notify(handle, launched ? "Copied " + text : "Sampled " + text + "; install wl-clipboard to copy",
               launched ? ICON_OK : ICON_WARNING, color, 1800);
    });
}
void ClipboardWriter::cancel() {
    m_pending.reset();
}
}
