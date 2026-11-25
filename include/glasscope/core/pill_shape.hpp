#pragma once
#include "glasscope/math/vector.hpp"

namespace Glasscope {

// Stretches the round lens into a pill (a capsule): the lens circle swept along a
// segment of half-length `half` through the lens centre, in direction `axis`.
// half == 0 is the plain circle. Units are logical pixels, relative to the centre.
//
// Gesture (pinned lens): pulling a round lens first behaves like the original jelly
// (the body shrinks and a second blob follows the pointer). Once the pointer has been
// dragged SNAP_FACTOR * radius from where it grabbed, the jelly lets go and the lens
// snaps out into a pill that then follows the pointer. An existing pill is simply
// lengthened or shortened by dragging it.
class PillShape {
  public:
    static constexpr double HALF_MAX = 1400.0;
    // How far (in lens radii) a round lens must be pulled before it snaps into a pill.
    static constexpr double SNAP_FACTOR = 1.5;

    enum class Drag { Idle, Jelly, Stretch };

    void reset();
    void advance(double elapsed);
    void settle();
    [[nodiscard]] bool needsAnimation() const noexcept;

    // The animated half-length: this is what is drawn and hit-tested.
    [[nodiscard]] double half() const noexcept {
        return m_half;
    }
    [[nodiscard]] bool active() const noexcept {
        return m_half > 0.0;
    }
    [[nodiscard]] Vec2 halfVector() const noexcept;
    [[nodiscard]] double distanceToCore(Vec2 point) const noexcept;

    // `point` is relative to the lens centre.
    void beginDrag(Vec2 point, double radius);
    // Returns true on the call where a round lens snaps into a pill.
    bool dragTo(Vec2 point);
    void endDrag() noexcept {
        m_drag = Drag::Idle;
    }
    [[nodiscard]] Drag drag() const noexcept {
        return m_drag;
    }
    [[nodiscard]] bool dragging() const noexcept {
        return m_drag != Drag::Idle;
    }
    [[nodiscard]] bool jelly() const noexcept {
        return m_drag == Drag::Jelly;
    }

  private:
    void aimAtPointer(Vec2 point);

    Vec2 m_axis = {1.0, 0.0};
    double m_half = 0.0;     // animated
    double m_velocity = 0.0; // of m_half
    double m_target = 0.0;   // where m_half is heading
    double m_sinceSnap = 1e9;
    Drag m_drag = Drag::Idle;
    bool m_fromCircle = false;
    double m_radius = 0.0;
    Vec2 m_pressPoint;
    double m_halfAtPress = 0.0;
    double m_projectionAtPress = 0.0;
};

}
