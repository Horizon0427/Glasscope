#pragma once

#include "glasscope/lens_motion.hpp"
#include "glasscope/lens_oscillation.hpp"
#include "glasscope/vector.hpp"

#include <array>
#include <cstdint>

namespace Glasscope {

struct LensSnapshot {
    Vec2 center;
    Vec2 velocity;
    Vec2 pullAxis = {1.0, 0.0};
    std::array<Vec2, 3> trailNodes;
    double pullShare = 0.28;
    double reveal = 0.0;
    double wobble = 0.0;
    double interactionWobble = 0.0;
    double phase = 0.0;
    bool requestedVisible = false;
    bool pinned = false;
};

class LensState {
  public:
    void setVisible(bool visible, Vec2 cursor, double nowSeconds);
    void toggle(Vec2 cursor, double nowSeconds);
    void move(Vec2 cursor, double nowSeconds);
    void setPinned(bool pinned, Vec2 position, double nowSeconds);
    bool beginPress(Vec2 cursor, double nowSeconds, double radius = 130.0);
    void endPress(double nowSeconds);
    void cancelPress();
    [[nodiscard]] bool pressed() const;
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
    bool m_pinned = false;
    bool m_pressed = false;
    bool m_dragged = false;
    Vec2 m_pressCursor;
    Vec2 m_pressPull;
    Vec2 m_pull;
    Vec2 m_pullVelocity;
    Vec2 m_pullAxis = {1.0, 0.0};
    double m_surfaceWave = 0.0;
    double m_surfaceVelocity = 0.0;
    bool m_reboundPending = false;
    uint32_t m_clickNoise = 0x6d2b79f5U;
    double m_clickGain = 1.0;
    double nextClickVariation();
    double m_contact = 0.0;
    double m_contactVelocity = 0.0;
    double m_lastPullTime = 0.0;
    double m_pullLimit = 234.0;
    double m_releaseStrength = 0.0;
    double m_pullShare = 0.28;
    double m_targetPullShare = 0.28;
};

}
