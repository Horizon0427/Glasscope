#include "glasscope/core/lens_state.hpp"
#include <algorithm>
#include <cmath>
namespace Glasscope {
void LensState::setVisible(bool visible, Vec2 cursor, double nowSeconds) {
    // Advance the old target first: hidden lenses stop ticking.
    advance(nowSeconds);
    if (!visible)
        cancelPress();
    if (m_pinned && m_motion.hasPointer())
        cursor = m_motion.center();
    const bool opening = visible && !m_visibility.requested();
    if (!m_motion.hasPointer() || opening) {
        m_motion.reset(cursor, nowSeconds);
        if (opening)
            m_oscillation.open(cursor, nowSeconds);
        else
            m_oscillation.syncPointer(cursor, nowSeconds);
    } else if (visible) {
        m_motion.setCenter(cursor);
    }

    m_visibility.request(visible);
}

void LensState::toggle(Vec2 cursor, double nowSeconds) {
    setVisible(!m_visibility.requested(), cursor, nowSeconds);
}

void LensState::move(Vec2 cursor, double nowSeconds) {
    if (m_pinned) {
        m_interaction.move(cursor, nowSeconds);
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
    m_pinned = pinned;
    m_interaction.resetForPin();
    m_motion.reset(position, nowSeconds);
    m_oscillation.settle(position, nowSeconds);
}
bool LensState::beginPress(Vec2 cursor, double nowSeconds, double radius) {
    if (!m_pinned || !m_visibility.requested() || pressed())
        return false;
    advance(nowSeconds);
    m_interaction.begin(cursor, m_motion.center(), nowSeconds, radius);
    m_oscillation.settle(m_motion.center(), nowSeconds);
    return true;
}
void LensState::endPress(double nowSeconds) {
    if (!pressed())
        return;
    advance(nowSeconds);
    m_interaction.end(nowSeconds);
    m_oscillation.settle(m_motion.center(), nowSeconds);
}
void LensState::cancelPress() {
    m_interaction.cancel();
}
bool LensState::pressed() const {
    return m_interaction.pressed();
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
        m_visibility.settle();
        m_motion.settle();
        m_oscillation.settle(m_motion.center(), nowSeconds);
        m_interaction.settleAfterGap();
        return;
    }
    const double elapsed = std::clamp(rawElapsed, 0.0, 0.05);
    m_visibility.advance(elapsed);
    m_motion.advance(nowSeconds, elapsed);
    m_oscillation.advance(nowSeconds, elapsed);
    m_interaction.advance(elapsed);
    if (m_visibility.finish()) {
        m_motion.settle();
        m_oscillation.settle(m_motion.center(), nowSeconds);
    }
    if (needsAnimation())
        m_phase = std::fmod(m_phase + elapsed, 1024.0);
}
void LensState::reset() {
    *this = {};
}
bool LensState::rendering() const {
    return m_visibility.rendering();
}
bool LensState::needsAnimation() const {
    return m_visibility.needsAnimation() || m_motion.needsAnimation() || m_oscillation.needsAnimation() ||
           m_interaction.needsAnimation();
}
LensSnapshot LensState::snapshot() const {
    const auto interaction = m_interaction.snapshot();
    return {.center = m_motion.center(),
            .velocity = m_motion.velocity(),
            .pullAxis = interaction.axis,
            .trailNodes = m_pinned ? interaction.trail : m_motion.trailOffsets(),
            .pullShare = interaction.share,
            .reveal = m_visibility.value(),
            .wobble = m_oscillation.value() + interaction.surfaceWave,
            .interactionWobble = interaction.interactionWobble,
            .phase = m_phase,
            .requestedVisible = m_visibility.requested(),
            .pinned = m_pinned};
}
}
