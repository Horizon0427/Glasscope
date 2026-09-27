#pragma once

#include "glasscope/core/lens_motion.hpp"
#include "glasscope/core/lens_oscillation.hpp"
#include "glasscope/model/lens_snapshot.hpp"

#include "glasscope/core/pinned_interaction.hpp"
#include "glasscope/core/visibility.hpp"
#include <array>

namespace Glasscope {

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
    PinnedInteraction m_interaction;
    Visibility m_visibility;
    double m_phase = 0.0;
    double m_lastUpdate = 0.0;
    bool m_hasClock = false;
    bool m_pinned = false;
};
}
