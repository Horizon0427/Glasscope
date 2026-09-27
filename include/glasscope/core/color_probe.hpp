#pragma once
#include "glasscope/core/lens_state.hpp"
#include "glasscope/model/color_sample.hpp"
#include <array>
#include <cstdint>
#include <optional>
namespace Glasscope {
enum class ProbePhase { Idle, Aiming, CapturePending, Feedback };
struct ProbeVisual {
    float amount = 0.0F;
    float captured = 0.0F;
    std::array<float, 3> color = {};
    bool capture = false;
};
class ColorProbe {
  public:
    bool begin(LensState& lens, Vec2 cursor, double now);
    bool pick(const LensState& lens);
    bool complete(const LensState& lens, std::uint64_t request, ColorSample sample, double now);
    bool cancel(LensState& lens, double now);
    bool expire(LensState& lens, double now);
    [[nodiscard]] std::uint64_t requestId() const {
        return m_request;
    }
    [[nodiscard]] ProbePhase phase() const {
        return m_phase;
    }
    [[nodiscard]] bool aiming() const {
        return m_phase == ProbePhase::Aiming;
    }
    [[nodiscard]] bool capturePending() const {
        return m_phase == ProbePhase::CapturePending;
    }
    [[nodiscard]] bool blocksPinnedPress() const {
        return capturePending() || m_phase == ProbePhase::Feedback;
    }
    [[nodiscard]] float feedbackAmount(double now) const;
    [[nodiscard]] ProbeVisual visual(double now) const;

  private:
    void restorePinned(LensState& lens, double now);
    ProbePhase m_phase = ProbePhase::Idle;
    std::uint64_t m_request = 0;
    std::optional<Vec2> m_pinnedCenter;
    double m_feedbackUntil = 0.0;
    std::array<float, 3> m_color = {};
};
}
