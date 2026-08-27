#pragma once

#include "glasscope/lens_motion.hpp"
#include "glasscope/lens_oscillation.hpp"
#include "glasscope/vector.hpp"

#include <array>

namespace Glasscope {

struct LensSnapshot {
    Vec2 center;
    Vec2 velocity;
    std::array<Vec2, 3> trailNodes;
    double reveal = 0.0;
    double wobble = 0.0;
    double phase = 0.0;
    bool requestedVisible = false;
};

class LensState {
  public:
    void setVisible(bool visible, Vec2 cursor, double nowSeconds);
    void toggle(Vec2 cursor, double nowSeconds);
    void move(Vec2 cursor, double nowSeconds);
    void advance(double nowSeconds);
    void reset();

    [[nodiscard]] bool rendering() const;
    [[nodiscard]] bool needsAnimation() const;
    [[nodiscard]] LensSnapshot snapshot() const;

  private:
    LensMotion m_motion;
    LensOscillation m_oscillation;
    double m_reveal = 0.0;
    double m_phase = 0.0;
    double m_lastUpdate = 0.0;
    bool m_requestedVisible = false;
    bool m_hasClock = false;
};

} // namespace Glasscope
