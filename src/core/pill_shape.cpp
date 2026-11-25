#include "glasscope/core/pill_shape.hpp"
#include "glasscope/math/damped_spring.hpp"
#include <algorithm>
#include <cmath>

namespace Glasscope {

namespace {

constexpr double SNAP_DEGREES = 8.0;
constexpr double MIN_HALF = 2.0;
// The pill pops out this far (in radii) at the snap even if the pointer is nearer the centre.
constexpr double MIN_SNAP_HALF_FACTOR = 0.35;
// Underdamped for a moment after the snap (a visible overshoot), then tight tracking.
constexpr double SNAP_WINDOW = 0.35;
constexpr double SNAP_DAMPING = 12.0;
constexpr double SNAP_FREQUENCY = 38.0;
constexpr double TRACK_DAMPING = 34.0;

double dot(Vec2 a, Vec2 b) noexcept {
    return a.x * b.x + a.y * b.y;
}

// Pull a direction onto the horizontal/vertical axis when it is within a few degrees.
Vec2 snapAxis(Vec2 unit) noexcept {
    const double pi = std::acos(-1.0);
    const double angle = std::atan2(unit.y, unit.x);
    const double quarter = pi / 2.0;
    const double nearest = std::round(angle / quarter) * quarter;
    if (std::abs(angle - nearest) < SNAP_DEGREES * pi / 180.0)
        return {std::round(std::cos(nearest)), std::round(std::sin(nearest))};
    return unit;
}

double clampHalf(double half) noexcept {
    if (half < MIN_HALF)
        return 0.0;
    return std::min(half, PillShape::HALF_MAX);
}

}

void PillShape::reset() {
    *this = {};
}

void PillShape::advance(double elapsed) {
    m_sinceSnap += elapsed;
    if (m_sinceSnap < SNAP_WINDOW)
        advanceDampedSpring(m_half, m_velocity, m_target, SNAP_DAMPING, SNAP_FREQUENCY, elapsed);
    else
        advanceDampedSpring(m_half, m_velocity, m_target, TRACK_DAMPING, 0.0, elapsed);
    m_half = std::clamp(m_half, 0.0, HALF_MAX);
    if (std::abs(m_half - m_target) < 0.05 && std::abs(m_velocity) < 0.5) {
        m_half = m_target;
        m_velocity = 0.0;
    }
}

void PillShape::settle() {
    m_half = m_target;
    m_velocity = 0.0;
}

bool PillShape::needsAnimation() const noexcept {
    return std::abs(m_half - m_target) > 0.05 || std::abs(m_velocity) > 0.5;
}

Vec2 PillShape::halfVector() const noexcept {
    return multiply(m_axis, m_half);
}

double PillShape::distanceToCore(Vec2 point) const noexcept {
    if (m_half <= 0.0)
        return length(point);
    const double along = std::clamp(dot(point, m_axis), -m_half, m_half);
    return length(subtract(point, multiply(m_axis, along)));
}

void PillShape::beginDrag(Vec2 point, double radius) {
    m_radius = radius;
    m_pressPoint = point;
    m_halfAtPress = m_target;
    m_projectionAtPress = std::abs(dot(point, m_axis));
    m_fromCircle = m_target <= 0.0;
    // A round lens is pulled like the original jelly until it snaps; a pill is stretched directly.
    m_drag = m_fromCircle ? Drag::Jelly : Drag::Stretch;
}

// The axis follows the pointer (snapping to horizontal/vertical when close), the near
// cap's outer edge sits under the pointer.
void PillShape::aimAtPointer(Vec2 point) {
    const double distance = length(point);
    // Close to the centre the direction is meaningless: use the drag direction instead.
    const Vec2 basis = distance > 0.2 * m_radius ? point : subtract(point, m_pressPoint);
    const double basisLength = length(basis);
    if (basisLength > 1.0)
        m_axis = snapAxis(multiply(basis, 1.0 / basisLength));
    m_target = clampHalf(std::abs(dot(point, m_axis)) - m_radius);
}

bool PillShape::dragTo(Vec2 point) {
    if (m_drag == Drag::Idle)
        return false;
    if (m_drag == Drag::Jelly) {
        if (length(subtract(point, m_pressPoint)) < SNAP_FACTOR * m_radius)
            return false;
        // Snap: the jelly lets go and the lens pops out into a pill under the pointer.
        m_drag = Drag::Stretch;
        m_sinceSnap = 0.0;
        aimAtPointer(point);
        m_target = std::max(m_target, MIN_SNAP_HALF_FACTOR * m_radius);
        return true;
    }
    if (m_fromCircle) {
        aimAtPointer(point);
        return false;
    }
    // Existing pill: keep its direction, change its length by how far the pointer has
    // moved along the axis since the press.
    const double projection = std::abs(dot(point, m_axis));
    m_target = clampHalf(m_halfAtPress + (projection - m_projectionAtPress));
    return false;
}

}
