#pragma once
#include "glasscope/core/lens_state.hpp"
namespace Glasscope {
enum class PressOwner { None, PinnedLens, ColorProbe };
struct InputContext {
    bool enabled = false;
    bool cancelled = false;
    bool locked = false;
};
struct InputResult {
    bool consume = false;
    bool changed = false;
    bool captureColor = false;
    bool accepted = false;
};
class InputRouter {
  public:
    InputResult move(LensState& lens, Vec2 cursor, double now, InputContext context);
    InputResult button(LensState& lens, Vec2 cursor, double now, InputContext context, bool left, bool down,
                       bool probeAiming, bool hit, double radius);
    void cancelForLock(LensState& lens);
    [[nodiscard]] bool hasCapture() const {
        return m_owner != PressOwner::None;
    }
    [[nodiscard]] PressOwner owner() const {
        return m_owner;
    }

  private:
    PressOwner m_owner = PressOwner::None;
};
}
