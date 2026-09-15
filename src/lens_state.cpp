#include "glasscope/lens_state.hpp"

#include <algorithm>
#include <cmath>

namespace Glasscope {

void LensState::setVisible(bool visible, Vec2 cursor, double nowSeconds) {
    if (!m_hasClock) {
        m_lastUpdate = nowSeconds;
        m_hasClock = true;
    }

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
        return;
    }

    const double elapsed = std::clamp(rawElapsed, 0.0, 0.05);
    const double target = m_requestedVisible ? 1.0 : 0.0;
    const double revealRate = m_requestedVisible ? 18.0 : 23.0;
    const double revealMix = 1.0 - std::exp(-elapsed * revealRate);
    m_reveal += (target - m_reveal) * revealMix;

    m_motion.advance(nowSeconds, elapsed);
    m_oscillation.advance(nowSeconds, elapsed);

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
    return std::abs(target - m_reveal) > 0.002 || m_motion.needsAnimation() ||
           m_oscillation.needsAnimation();
}

LensSnapshot LensState::snapshot() const {
    return {
        .center = m_motion.center(),
        .velocity = m_motion.velocity(),
        .trailNodes = m_motion.trailOffsets(),
        .reveal = m_reveal,
        .wobble = m_oscillation.value(),
        .phase = m_phase,
        .requestedVisible = m_requestedVisible,
    };
}

}
