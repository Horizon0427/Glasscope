#include "glasscope/core/input_router.hpp"
namespace Glasscope {
void InputRouter::cancelForLock(LensState& lens) {
    lens.cancelPress();
    m_owner = PressOwner::None;
}
InputResult InputRouter::move(LensState& lens, Vec2 cursor, double now, InputContext context) {
    if (context.locked) {
        cancelForLock(lens);
        return {};
    }
    if (!context.enabled || context.cancelled)
        return {};
    const auto before = lens.snapshot();
    const bool changed = (before.requestedVisible || lens.rendering()) && (!before.pinned || lens.pressed());
    if (changed)
        lens.move(cursor, now);
    return {.consume = lens.pressed(), .changed = changed, .accepted = true};
}
InputResult InputRouter::button(LensState& lens, Vec2 cursor, double now, InputContext context, bool left, bool down,
                                bool probeAiming, bool hit, double radius) {
    if (!left)
        return {};
    if (context.locked) {
        cancelForLock(lens);
        return {};
    }
    // Consume the owned release even after hide/disable or upstream cancellation.
    if (hasCapture() && !down) {
        m_owner = PressOwner::None;
        lens.endPress(now);
        return {.consume = true, .changed = true, .accepted = true};
    }
    if (!context.enabled || context.cancelled)
        return {};
    if (probeAiming && down && lens.snapshot().requestedVisible) {
        m_owner = PressOwner::ColorProbe;
        return {.consume = true, .captureColor = true, .accepted = true};
    }
    if (down && hit && lens.beginPress(cursor, now, radius)) {
        m_owner = PressOwner::PinnedLens;
        return {.consume = true, .changed = true, .accepted = true};
    }
    return {.accepted = true};
}
}
