#pragma once

#include "glasscope/vector.hpp"

namespace Glasscope {

class LensOscillation {
  public:
    void syncPointer(Vec2 cursor, double nowSeconds);
    void open(Vec2 cursor, double nowSeconds);
    void impulse(Vec2 cursor, double nowSeconds, double strength);
    void observeMotion(Vec2 cursor, double speed, double nowSeconds);
    void advance(double nowSeconds, double elapsed);
    void settle(Vec2 cursor, double nowSeconds);

    [[nodiscard]] bool needsAnimation() const;
    [[nodiscard]] double value() const;

  private:
    void resetDynamics(Vec2 cursor, double nowSeconds);

    Vec2 m_motionAnchor;
    double m_strength = 0.0;
    double m_age = -1.0;
    double m_lastMotion = 0.0;
    double m_value = 0.0;
    bool m_pending = false;
};

}
