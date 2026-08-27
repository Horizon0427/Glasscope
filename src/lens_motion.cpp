#include "glasscope/lens_motion.hpp"

#include <algorithm>
#include <cmath>

namespace Glasscope {
namespace {

constexpr std::array<double, 3> FOLLOW_RATES = {25.0, 17.0, 11.5};

} // namespace

void LensMotion::reset(Vec2 cursor, double nowSeconds) {
    m_center = cursor;
    m_velocity = {};
    m_trailPositions.fill(cursor);
    m_lastMotion = nowSeconds;
    m_hasPointer = true;
}

MotionUpdate LensMotion::move(Vec2 cursor, double nowSeconds) {
    if (!m_hasPointer) {
        reset(cursor, nowSeconds);
        return MotionUpdate::Initialized;
    }

    const double rawElapsed = nowSeconds - m_lastMotion;
    if (rawElapsed > 0.20) {
        reset(cursor, nowSeconds);
        return MotionUpdate::SettledAfterGap;
    }

    const double elapsed = std::clamp(rawElapsed, 0.001, 0.1);
    const Vec2 instantaneous = clampLength({
        (cursor.x - m_center.x) / elapsed,
        (cursor.y - m_center.y) / elapsed,
    }, 5200.0);

    const double velocityResponse = 1.0 - std::exp(-elapsed * 88.0);
    m_velocity = mix(m_velocity, instantaneous, velocityResponse);
    m_center = cursor;
    m_lastMotion = nowSeconds;
    return MotionUpdate::Updated;
}

void LensMotion::setCenter(Vec2 cursor) {
    m_center = cursor;
}

void LensMotion::advance(double nowSeconds, double elapsed) {
    for (std::size_t index = 0; index < m_trailPositions.size(); ++index) {
        const Vec2 delta = subtract(m_center, m_trailPositions[index]);
        const double distance = length(delta);
        const double distanceResponse = 5.0 * std::clamp(distance / 180.0, 0.0, 1.0);
        const double followAmount = 1.0 - std::exp(-elapsed * (FOLLOW_RATES[index] + distanceResponse));
        m_trailPositions[index] = mix(m_trailPositions[index], m_center, followAmount);
        if (distance < 0.18)
            m_trailPositions[index] = m_center;
    }

    const double idleElapsed = std::clamp(nowSeconds - m_lastMotion, 0.0, elapsed);
    m_velocity = multiply(m_velocity, std::exp(-idleElapsed * 18.0));
}

void LensMotion::settle() {
    m_velocity = {};
    m_trailPositions.fill(m_center);
}

bool LensMotion::hasPointer() const {
    return m_hasPointer;
}

bool LensMotion::needsAnimation() const {
    double maximumTrailDistance = 0.0;
    for (const Vec2 position : m_trailPositions)
        maximumTrailDistance = std::max(maximumTrailDistance, length(subtract(position, m_center)));
    return length(m_velocity) > 2.0 || maximumTrailDistance > 0.25;
}

Vec2 LensMotion::center() const {
    return m_center;
}

Vec2 LensMotion::velocity() const {
    return m_velocity;
}

std::array<Vec2, 3> LensMotion::trailOffsets() const {
    return {
        subtract(m_trailPositions[0], m_center),
        subtract(m_trailPositions[1], m_center),
        subtract(m_trailPositions[2], m_center),
    };
}

} // namespace Glasscope
