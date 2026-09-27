#include "glasscope/lens_state.hpp"

#include <algorithm>
#include <cmath>

namespace Glasscope {
namespace {
// Match the following lens's quick surface rhythm, with a slightly softer tail.
constexpr double SURFACE_FREQUENCY = 31.0;
constexpr double SURFACE_DAMPING = 6.8;

// Exact spring evolution carries position and velocity across release and
// re-grab. Damping is the exponential decay rate; frequency is damped rad/s.
void springStep(double& position, double& velocity, double target, double damping, double frequency, double elapsed) {
    const double offset = position - target;
    const double decay = std::exp(-damping * elapsed);
    if (frequency <= 0.0) {
        const double momentum = velocity + damping * offset;
        position = target + decay * (offset + momentum * elapsed);
        velocity = decay * (velocity - damping * momentum * elapsed);
        return;
    }
    const double cosine = std::cos(frequency * elapsed);
    const double sine = std::sin(frequency * elapsed);
    position = target + decay * (offset * cosine + (velocity + damping * offset) / frequency * sine);
    velocity = decay * (velocity * cosine -
                        (damping * velocity + (damping * damping + frequency * frequency) * offset) / frequency * sine);
}
}

double LensState::nextClickVariation() {
    // Event-local, reproducible noise: never sample randomness per frame.
    m_clickNoise ^= m_clickNoise << 13;
    m_clickNoise ^= m_clickNoise >> 17;
    m_clickNoise ^= m_clickNoise << 5;
    return static_cast<double>(m_clickNoise) / 4294967295.0;
}

void LensState::setVisible(bool visible, Vec2 cursor, double nowSeconds) {
    // Settle elapsed time against the old visibility target before starting a
    // transition. Hidden lenses do not tick, so their clock can be arbitrarily
    // old; carrying that gap into the first frame would skip the new animation.
    advance(nowSeconds);
    if (!visible)
        cancelPress();
    if (m_pinned && m_motion.hasPointer())
        cursor = m_motion.center();
    const bool opening = visible && !m_requestedVisible;
    if (!m_motion.hasPointer() || opening) {
        m_motion.reset(cursor, nowSeconds);
        if (opening)
            m_oscillation.open(cursor, nowSeconds);
        else
            m_oscillation.syncPointer(cursor, nowSeconds);
    } else if (visible) {
        m_motion.setCenter(cursor);
    }

    m_requestedVisible = visible;
}

void LensState::toggle(Vec2 cursor, double nowSeconds) {
    setVisible(!m_requestedVisible, cursor, nowSeconds);
}

void LensState::move(Vec2 cursor, double nowSeconds) {
    if (m_pinned) {
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
    switch (m_motion.move(cursor, nowSeconds)) {
    case MotionUpdate::Initialized:
        m_oscillation.syncPointer(cursor, nowSeconds);
        return;
    case MotionUpdate::SettledAfterGap:
        m_oscillation.settle(cursor, nowSeconds);
        return;
    case MotionUpdate::Updated:
        m_oscillation.observeMotion(cursor, length(m_motion.velocity()), nowSeconds);
        return;
    }
}

void LensState::setPinned(bool pinned, Vec2 position, double nowSeconds) {
    cancelPress();
    m_pinned = pinned;
    m_pull = {};
    m_pullVelocity = {};
    m_motion.reset(position, nowSeconds);
    m_contact = m_contactVelocity = 0.0;
    m_surfaceWave = m_surfaceVelocity = 0.0;
    m_pullShare = m_targetPullShare = 0.28;
    m_pullAxis = {1.0, 0.0};
    m_oscillation.settle(position, nowSeconds);
}

bool LensState::beginPress(Vec2 cursor, double nowSeconds, double radius) {
    if (!m_pinned || !m_requestedVisible || m_pressed)
        return false;
    advance(nowSeconds);
    m_pressed = true;
    m_dragged = false;
    m_reboundPending = false;
    const double clickAngle = nextClickVariation() * std::acos(-1.0);
    m_clickGain = 0.92 + 0.16 * nextClickVariation();
    // Only rotate a settled shape. A re-grab must keep the visible contour.
    if (length(m_pull) < 0.08 && std::abs(m_contact) < 0.001 && std::abs(m_contactVelocity) < 0.02 &&
        std::abs(m_surfaceWave) < 0.001 && std::abs(m_surfaceVelocity) < 0.02)
        m_pullAxis = {std::cos(clickAngle), std::sin(clickAngle)};
    m_pressCursor = cursor;
    m_pressPull = m_pull;
    m_pullVelocity = {};
    m_lastPullTime = nowSeconds;
    m_pullLimit = std::max(radius * 1.8, 1.0);
    const double depth =
        std::clamp(1.0 - length(subtract(cursor, m_motion.center())) / std::max(radius, 1.0), 0.0, 1.0);
    m_targetPullShare = 0.06 + 0.44 * depth * depth * (3.0 - 2.0 * depth);
    if (length(m_pull) < 0.08)
        m_pullShare = m_targetPullShare;
    m_oscillation.settle(m_motion.center(), nowSeconds);
    return true;
}

void LensState::endPress(double nowSeconds) {
    if (!m_pressed)
        return;
    advance(nowSeconds);
    const bool dragged = m_dragged;
    m_releaseStrength = std::clamp(length(m_pull) / m_pullLimit, 0.0, 1.0);
    // Transfer only recent velocity along the held axis. Pausing before release
    // must not launch the lens with stale pointer momentum.
    const double freshness = std::exp(-std::max(nowSeconds - m_lastPullTime, 0.0) / 0.035);
    const double releaseSpeed =
        std::clamp((m_pullVelocity.x * m_pullAxis.x + m_pullVelocity.y * m_pullAxis.y) * freshness, -m_pullLimit * 4.0,
                   m_pullLimit * 4.0);
    m_pullVelocity = multiply(m_pullAxis, releaseSpeed);
    cancelPress();
    m_oscillation.settle(m_motion.center(), nowSeconds);
    m_reboundPending = dragged && (m_pull.x * m_pullAxis.x + m_pull.y * m_pullAxis.y) > 0.08;
    if (!dragged)
        m_contactVelocity = std::min(m_contactVelocity + 20.0 * m_clickGain, 24.0);
}

void LensState::cancelPress() {
    m_reboundPending = false;
    m_pressed = false;
    m_dragged = false;
}

bool LensState::pressed() const {
    return m_pressed;
}

void LensState::advance(double nowSeconds) {
    if (!m_hasClock) {
        m_lastUpdate = nowSeconds;
        m_hasClock = true;
        return;
    }

    const double rawElapsed = nowSeconds - m_lastUpdate;
    m_lastUpdate = nowSeconds;

    if (rawElapsed > 0.20) {
        m_reveal = m_requestedVisible ? 1.0 : 0.0;
        m_motion.settle();
        m_oscillation.settle(m_motion.center(), nowSeconds);
        m_contact = m_pressed ? -0.10 : 0.0;
        m_contactVelocity = 0.0;
        m_surfaceWave = m_surfaceVelocity = 0.0;
        m_reboundPending = false;
        m_pullShare = m_targetPullShare;
        if (!m_pressed) {
            m_pull = {};
            m_pullVelocity = {};
        }
        return;
    }

    const double elapsed = std::clamp(rawElapsed, 0.0, 0.05);
    const double target = m_requestedVisible ? 1.0 : 0.0;
    const double revealRate = m_requestedVisible ? 18.0 : 23.0;
    const double revealMix = 1.0 - std::exp(-elapsed * revealRate);
    m_reveal += (target - m_reveal) * revealMix;

    m_motion.advance(nowSeconds, elapsed);
    m_oscillation.advance(nowSeconds, elapsed);

    // Re-grabbing a moving surface blends toward the newly captured share.
    m_pullShare += (m_targetPullShare - m_pullShare) * (1.0 - std::exp(-elapsed * 32.0));
    if (std::abs(m_pullShare - m_targetPullShare) < 0.0001)
        m_pullShare = m_targetPullShare;
    const double contactTarget = m_pressed ? -0.10 : 0.0;
    springStep(m_contact, m_contactVelocity, contactTarget, m_pressed ? 60.0 : 6.5, m_pressed ? 0.0 : 23.0, elapsed);
    if (std::abs(m_contact - contactTarget) < 0.0001 && std::abs(m_contactVelocity) < 0.002) {
        m_contact = contactTarget;
        m_contactVelocity = 0.0;
    }

    springStep(m_surfaceWave, m_surfaceVelocity, 0.0, SURFACE_DAMPING, SURFACE_FREQUENCY, elapsed);

    if (!m_pressed && (length(m_pull) > 0.0 || length(m_pullVelocity) > 0.0)) {
        // A slightly slower, less damped return leaves room for a readable
        // squash and secondary bounce instead of snapping straight to rest.
        const double damping = 13.0 - 6.0 * m_releaseStrength;
        const double frequency = 17.0 + 3.0 * m_releaseStrength;
        const double oldAlong = m_pull.x * m_pullAxis.x + m_pull.y * m_pullAxis.y;
        const double oldSpeed = m_pullVelocity.x * m_pullAxis.x + m_pullVelocity.y * m_pullAxis.y;
        springStep(m_pull.x, m_pullVelocity.x, 0.0, damping, frequency, elapsed);
        springStep(m_pull.y, m_pullVelocity.y, 0.0, damping, frequency, elapsed);
        const double along = m_pull.x * m_pullAxis.x + m_pull.y * m_pullAxis.y;
        if (m_reboundPending && oldAlong > 0.0 && along <= 0.0) {
            // Solve the first zero crossing inside this frame, then evolve the
            // impulse for the remaining time. This keeps the handoff independent
            // of whether the compositor rendered 30 or 240 frames per second.
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

    if (!m_requestedVisible && m_reveal < 0.002) {
        m_reveal = 0.0;
        m_motion.settle();
        m_oscillation.settle(m_motion.center(), nowSeconds);
    } else if (m_requestedVisible && m_reveal > 0.998) {
        m_reveal = 1.0;
    }

    if (needsAnimation())
        m_phase = std::fmod(m_phase + elapsed, 1024.0);
}

void LensState::reset() {
    *this = {};
}

bool LensState::rendering() const {
    return m_reveal > 0.001;
}

bool LensState::needsAnimation() const {
    const double target = m_requestedVisible ? 1.0 : 0.0;
    return std::abs(target - m_reveal) > 0.002 || m_motion.needsAnimation() || m_oscillation.needsAnimation() ||
           std::abs(m_surfaceWave) > 0.0001 || std::abs(m_surfaceVelocity) > 0.002 ||
           std::abs(m_pullShare - m_targetPullShare) > 0.0001 ||
           std::abs(m_contact - (m_pressed ? -0.10 : 0.0)) > 0.0001 || std::abs(m_contactVelocity) > 0.002 ||
           (!m_pressed && (length(m_pull) > 0.0 || length(m_pullVelocity) > 0.0));
}

LensSnapshot LensState::snapshot() const {
    const double compression = std::min((m_pull.x * m_pullAxis.x + m_pull.y * m_pullAxis.y) / m_pullLimit, 0.0);
    // Zero slope at the merge avoids a hard switch from lobe motion to squash.
    const double recoil = 4.0 * compression * (1.0 - std::exp(compression / 0.025));
    return {
        .center = m_motion.center(),
        .velocity = m_motion.velocity(),
        .pullAxis = m_pullAxis,
        .trailNodes = m_pinned ? std::array<Vec2, 3>{multiply(m_pull, 0.33), multiply(m_pull, 0.66), m_pull}
                               : m_motion.trailOffsets(),
        .pullShare = m_pullShare,
        .reveal = m_reveal,
        .wobble = m_oscillation.value() + m_surfaceWave,
        .interactionWobble = m_contact + recoil,
        .phase = m_phase,
        .requestedVisible = m_requestedVisible,
        .pinned = m_pinned,
    };
}

}
