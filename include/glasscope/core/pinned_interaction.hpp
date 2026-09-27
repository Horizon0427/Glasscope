#pragma once
#include "glasscope/math/vector.hpp"
#include <array>
#include <cstdint>
namespace Glasscope {
struct PinnedSnapshot {
    Vec2 axis;
    std::array<Vec2, 3> trail;
    double share;
    double surfaceWave;
    double interactionWobble;
};
class PinnedInteraction {
  public:
    void resetForPin();
    void begin(Vec2 cursor, Vec2 center, double nowSeconds, double radius);
    void move(Vec2 cursor, double nowSeconds);
    void end(double nowSeconds);
    void cancel();
    void advance(double elapsed);
    void settleAfterGap();
    [[nodiscard]] bool pressed() const {
        return m_pressed;
    }
    [[nodiscard]] bool needsAnimation() const;
    [[nodiscard]] PinnedSnapshot snapshot() const;

  private:
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
