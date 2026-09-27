#include "glasscope/core/color_probe.hpp"
#include <algorithm>
namespace Glasscope {
namespace {
constexpr double FEEDBACK_DURATION = 1.05;
constexpr double FADE_DURATION = 0.22;
}
bool ColorProbe::begin(LensState& lens, Vec2 cursor, double now) {
    const auto before = lens.snapshot();
    if (!before.requestedVisible)
        return false;
    if (before.pinned) {
        m_pinnedCenter = before.center;
        lens.setPinned(false, cursor, now);
    }
    ++m_request;
    m_phase = ProbePhase::Aiming;
    m_feedbackUntil = 0.0;
    return true;
}
bool ColorProbe::pick(const LensState& lens) {
    if (!lens.snapshot().requestedVisible)
        return false;
    ++m_request;
    m_phase = ProbePhase::CapturePending;
    m_feedbackUntil = 0.0;
    return true;
}
bool ColorProbe::complete(const LensState& lens, std::uint64_t request, ColorSample sample, double now) {
    if (!lens.snapshot().requestedVisible || !capturePending() || request != m_request)
        return false;
    m_phase = ProbePhase::Feedback;
    m_color = {static_cast<float>(sample.red) / 255.0F, static_cast<float>(sample.green) / 255.0F,
               static_cast<float>(sample.blue) / 255.0F};
    m_feedbackUntil = now + FEEDBACK_DURATION;
    return true;
}
bool ColorProbe::cancel(LensState& lens, double now) {
    const bool hadProbe = m_phase != ProbePhase::Idle;
    ++m_request;
    m_phase = ProbePhase::Idle;
    m_feedbackUntil = 0.0;
    restorePinned(lens, now);
    return hadProbe;
}
bool ColorProbe::expire(LensState& lens, double now) {
    if (m_phase != ProbePhase::Feedback || m_feedbackUntil > now)
        return false;
    ++m_request;
    m_phase = ProbePhase::Idle;
    m_feedbackUntil = 0.0;
    restorePinned(lens, now);
    return true;
}
void ColorProbe::restorePinned(LensState& lens, double now) {
    if (!m_pinnedCenter)
        return;
    lens.setPinned(true, *m_pinnedCenter, now);
    m_pinnedCenter.reset();
}
float ColorProbe::feedbackAmount(double now) const {
    if (m_feedbackUntil <= now)
        return 0.0F;
    return static_cast<float>(std::clamp((m_feedbackUntil - now) / FADE_DURATION, 0.0, 1.0));
}
ProbeVisual ColorProbe::visual(double now) const {
    const float feedback = feedbackAmount(now);
    return {.amount = aiming() || capturePending() ? 1.0F : feedback,
            .captured = feedback > 0.0F && !aiming() && !capturePending() ? 1.0F : 0.0F,
            .color = m_color,
            .capture = capturePending()};
}
}
