#pragma once

#include "glasscope/math/vector.hpp"

#include <array>
#include <cstdint>

namespace Glasscope {

enum class MotionUpdate : std::uint8_t {
    Initialized,
    SettledAfterGap,
    Updated,
};

class LensMotion {
  public:
    void reset(Vec2 cursor, double nowSeconds);
    MotionUpdate move(Vec2 cursor, double nowSeconds);
    void setCenter(Vec2 cursor);
    void advance(double nowSeconds, double elapsed);
    void settle();

    [[nodiscard]] bool hasPointer() const;
    [[nodiscard]] bool needsAnimation() const;
    [[nodiscard]] Vec2 center() const;
    [[nodiscard]] Vec2 velocity() const;
    [[nodiscard]] std::array<Vec2, 3> trailOffsets() const;

  private:
    Vec2 m_center;
    Vec2 m_velocity;
    std::array<Vec2, 3> m_trailPositions;
    double m_lastMotion = 0.0;
    bool m_hasPointer = false;
};

}
