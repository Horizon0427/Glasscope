#include "glasscope/core/pinned_interaction.hpp"
#include "glasscope/math/damped_spring.hpp"
#include <algorithm>
#include <cmath>
namespace Glasscope {
namespace {
constexpr double SURFACE_FREQUENCY = 31.0;
constexpr double SURFACE_DAMPING = 6.8;
}
double PinnedInteraction::nextClickVariation() {
    // Sample once per gesture, never per frame.
    m_clickNoise ^= m_clickNoise << 13;
    m_clickNoise ^= m_clickNoise >> 17;
    m_clickNoise ^= m_clickNoise << 5;
    return static_cast<double>(m_clickNoise) / 4294967295.0;
}

void PinnedInteraction::resetForPin() {
    cancel();
    m_pull = {};
    m_pullVelocity = {};
    m_contact = m_contactVelocity = 0.0;
    m_surfaceWave = m_surfaceVelocity = 0.0;
    m_pullShare = m_targetPullShare = 0.28;
    m_pullAxis = {1.0, 0.0};
}
void PinnedInteraction::move(Vec2 cursor, double nowSeconds) {
    if (!m_pressed)
        return;
    const Vec2 delta = subtract(cursor, m_pressCursor);
    if (!m_dragged && length(delta) < 3.0)
        return;
    m_dragged = true;
    const Vec2 offset = multiply(delta, m_pullLimit / (m_pullLimit + length(delta)));
    const Vec2 nextPull = clampLength({m_pressPull.x + offset.x, m_pressPull.y + offset.y}, m_pullLimit);
    const double elapsed = nowSeconds - m_lastPullTime;
    if (elapsed > 0.00001 && elapsed < 0.10) {
        const Vec2 observed = clampLength(multiply(subtract(nextPull, m_pull), 1.0 / elapsed), m_pullLimit * 8.0);
        const double blend = 1.0 - std::exp(-elapsed * 40.0);
        m_pullVelocity = {m_pullVelocity.x + (observed.x - m_pullVelocity.x) * blend,
                          m_pullVelocity.y + (observed.y - m_pullVelocity.y) * blend};
    } else {
        m_pullVelocity = {};
    }
    m_pull = nextPull;
    m_lastPullTime = nowSeconds;
    if (length(m_pull) > 0.01)
        m_pullAxis = multiply(m_pull, 1.0 / length(m_pull));
    return;
}
void PinnedInteraction::begin(Vec2 cursor, Vec2 center, double nowSeconds, double radius) {
    m_pressed = true;
    m_dragged = false;
    m_reboundPending = false;
    const double clickAngle = nextClickVariation() * std::acos(-1.0);
    m_clickGain = 0.92 + 0.16 * nextClickVariation();
    // Re-grabs must preserve the current contour and axis.
    if (length(m_pull) < 0.08 && std::abs(m_contact) < 0.001 && std::abs(m_contactVelocity) < 0.02 &&
        std::abs(m_surfaceWave) < 0.001 && std::abs(m_surfaceVelocity) < 0.02)
        m_pullAxis = {std::cos(clickAngle), std::sin(clickAngle)};
    m_pressCursor = cursor;
    m_pressPull = m_pull;
    m_pullVelocity = {};
    m_lastPullTime = nowSeconds;
    m_pullLimit = std::max(radius * 1.8, 1.0);
    const double depth = std::clamp(1.0 - length(subtract(cursor, center)) / std::max(radius, 1.0), 0.0, 1.0);
    m_targetPullShare = 0.06 + 0.44 * depth * depth * (3.0 - 2.0 * depth);
    if (length(m_pull) < 0.08)
        m_pullShare = m_targetPullShare;
}

void PinnedInteraction::end(double nowSeconds) {
    const bool dragged = m_dragged;
    m_releaseStrength = std::clamp(length(m_pull) / m_pullLimit, 0.0, 1.0);
    // Decay stale pointer momentum before release.
    const double freshness = std::exp(-std::max(nowSeconds - m_lastPullTime, 0.0) / 0.035);
    const double releaseSpeed =
        std::clamp((m_pullVelocity.x * m_pullAxis.x + m_pullVelocity.y * m_pullAxis.y) * freshness, -m_pullLimit * 4.0,
                   m_pullLimit * 4.0);
    m_pullVelocity = multiply(m_pullAxis, releaseSpeed);
    cancel();
    m_reboundPending = dragged && (m_pull.x * m_pullAxis.x + m_pull.y * m_pullAxis.y) > 0.08;
    if (!dragged)
        m_contactVelocity = std::min(m_contactVelocity + 20.0 * m_clickGain, 24.0);
}

void PinnedInteraction::cancel() {
    m_reboundPending = false;
    m_pressed = false;
    m_dragged = false;
}

void PinnedInteraction::advance(double elapsed) {
    m_pullShare += (m_targetPullShare - m_pullShare) * (1.0 - std::exp(-elapsed * 32.0));
    if (std::abs(m_pullShare - m_targetPullShare) < 0.0001)
        m_pullShare = m_targetPullShare;
    const double contactTarget = m_pressed ? -0.10 : 0.0;
    advanceDampedSpring(m_contact, m_contactVelocity, contactTarget, m_pressed ? 60.0 : 6.5, m_pressed ? 0.0 : 23.0,
                        elapsed);
    if (std::abs(m_contact - contactTarget) < 0.0001 && std::abs(m_contactVelocity) < 0.002) {
        m_contact = contactTarget;
        m_contactVelocity = 0.0;
    }

    advanceDampedSpring(m_surfaceWave, m_surfaceVelocity, 0.0, SURFACE_DAMPING, SURFACE_FREQUENCY, elapsed);

    if (!m_pressed && (length(m_pull) > 0.0 || length(m_pullVelocity) > 0.0)) {
        const double damping = 13.0 - 6.0 * m_releaseStrength;
        const double frequency = 17.0 + 3.0 * m_releaseStrength;
        const double oldAlong = m_pull.x * m_pullAxis.x + m_pull.y * m_pullAxis.y;
        const double oldSpeed = m_pullVelocity.x * m_pullAxis.x + m_pullVelocity.y * m_pullAxis.y;
        advanceDampedSpring(m_pull.x, m_pullVelocity.x, 0.0, damping, frequency, elapsed);
        advanceDampedSpring(m_pull.y, m_pullVelocity.y, 0.0, damping, frequency, elapsed);
        const double along = m_pull.x * m_pullAxis.x + m_pull.y * m_pullAxis.y;
        if (m_reboundPending && oldAlong > 0.0 && along <= 0.0) {
            // Transfer at the within-frame zero crossing to keep the handoff frame-rate independent.
            double phase = std::atan2(-oldAlong * frequency, oldSpeed + damping * oldAlong);
            if (phase <= 0.0)
                phase += std::acos(-1.0);
            const double remaining = std::clamp(elapsed - phase / frequency, 0.0, elapsed);
            const double strength = 1.30 * (0.28 + 0.62 * m_releaseStrength);
            const double decay = std::exp(-SURFACE_DAMPING * remaining);
            const double angle = SURFACE_FREQUENCY * remaining;
            m_surfaceWave -= strength * decay * std::sin(angle);
            m_surfaceVelocity -=
                strength * decay * (SURFACE_FREQUENCY * std::cos(angle) - SURFACE_DAMPING * std::sin(angle));
            m_reboundPending = false;
        }
        if (length(m_pull) < 0.08 && length(m_pullVelocity) < 1.0) {
            m_pull = {};
            m_pullVelocity = {};
        }
    }

    if (std::abs(m_surfaceWave) < 0.0001 && std::abs(m_surfaceVelocity) < 0.002) {
        m_surfaceWave = m_surfaceVelocity = 0.0;
    }
}
void PinnedInteraction::settleAfterGap() {
    m_contact = m_pressed ? -0.10 : 0.0;
    m_contactVelocity = 0.0;
    m_surfaceWave = m_surfaceVelocity = 0.0;
    m_reboundPending = false;
    m_pullShare = m_targetPullShare;
    if (!m_pressed) {
        m_pull = {};
        m_pullVelocity = {};
    }
}
bool PinnedInteraction::needsAnimation() const {
    return std::abs(m_surfaceWave) > 0.0001 || std::abs(m_surfaceVelocity) > 0.002 ||
           std::abs(m_pullShare - m_targetPullShare) > 0.0001 ||
           std::abs(m_contact - (m_pressed ? -0.10 : 0.0)) > 0.0001 || std::abs(m_contactVelocity) > 0.002 ||
           (!m_pressed && (length(m_pull) > 0.0 || length(m_pullVelocity) > 0.0));
}
PinnedSnapshot PinnedInteraction::snapshot() const {
    const double compression = std::min((m_pull.x * m_pullAxis.x + m_pull.y * m_pullAxis.y) / m_pullLimit, 0.0);
    // Zero slope at the merge keeps recoil continuous.
    const double recoil = 4.0 * compression * (1.0 - std::exp(compression / 0.025));
    return {m_pullAxis,
            {multiply(m_pull, 0.33), multiply(m_pull, 0.66), m_pull},
            m_pullShare,
            m_surfaceWave,
            m_contact + recoil};
}
}
