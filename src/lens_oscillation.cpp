#include "glasscope/lens_oscillation.hpp"

#include <algorithm>
#include <cmath>

namespace Glasscope {
namespace {

constexpr double STOP_BOUNCE_DELAY = 0.09;
constexpr double STOP_BOUNCE_SPEED = 1600.0;
constexpr double STOP_BOUNCE_MIN_STRENGTH = 0.08;
constexpr double STOP_BOUNCE_MOTION_DISTANCE = 1.5;
constexpr double STOP_BOUNCE_ENERGY_DECAY = 2.4;
constexpr double STOP_BOUNCE_FREQUENCY = 31.0;
constexpr double STOP_BOUNCE_DAMPING = 7.5;
constexpr double STOP_BOUNCE_DURATION = 0.46;

}

void LensOscillation::syncPointer(Vec2 cursor, double nowSeconds) {
    resetDynamics(cursor, nowSeconds);
}

void LensOscillation::open(Vec2 cursor, double nowSeconds) {
    resetDynamics(cursor, nowSeconds);
    m_value = std::max(m_value, 0.34);
}

void LensOscillation::observeMotion(Vec2 cursor, double speed, double nowSeconds) {
    const double travel = length(subtract(cursor, m_motionAnchor));
    if (travel < STOP_BOUNCE_MOTION_DISTANCE)
        return;

    const double energyElapsed = std::max(nowSeconds - m_lastMotion, 0.0);
    const double retainedStrength = m_strength * std::exp(-energyElapsed * STOP_BOUNCE_ENERGY_DECAY);
    const double observedStrength = std::clamp(speed / STOP_BOUNCE_SPEED, 0.0, 1.0);
    m_strength = std::max(retainedStrength, observedStrength);
    m_pending = m_strength >= STOP_BOUNCE_MIN_STRENGTH;
    m_age = -1.0;
    m_value = 0.0;
    m_motionAnchor = cursor;
    m_lastMotion = nowSeconds;
}

void LensOscillation::impulse(Vec2 cursor, double nowSeconds, double strength) {
    resetDynamics(cursor, nowSeconds);
    m_strength = std::clamp(strength, 0.0, 0.8);
    m_age = 0.0;
    m_value = 0.0;
}

void LensOscillation::advance(double nowSeconds, double elapsed) {
    const double idleDuration = std::max(nowSeconds - m_lastMotion, 0.0);
    if (m_pending && idleDuration >= STOP_BOUNCE_DELAY) {
        m_pending = false;
        m_age = 0.0;
    }
    if (m_age >= 0.0) {
        m_age += elapsed;
        m_value = -m_strength * std::exp(-STOP_BOUNCE_DAMPING * m_age) * std::sin(STOP_BOUNCE_FREQUENCY * m_age);
        if (m_age >= STOP_BOUNCE_DURATION) {
            m_value = 0.0;
            m_strength = 0.0;
            m_age = -1.0;
        }
    } else {
        m_value *= std::exp(-elapsed * 14.0);
    }
}

void LensOscillation::settle(Vec2 cursor, double nowSeconds) {
    resetDynamics(cursor, nowSeconds);
    m_value = 0.0;
}

bool LensOscillation::needsAnimation() const {
    return std::abs(m_value) > 0.006 || m_pending || m_age >= 0.0;
}

double LensOscillation::value() const {
    return m_value;
}

void LensOscillation::resetDynamics(Vec2 cursor, double nowSeconds) {
    m_motionAnchor = cursor;
    m_strength = 0.0;
    m_age = -1.0;
    m_lastMotion = nowSeconds;
    m_pending = false;
}

}
