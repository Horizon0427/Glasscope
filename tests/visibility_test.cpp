#include "glasscope/lens_state.hpp"

#include <cmath>
#include <stdexcept>

using namespace Glasscope;

namespace {
void require(bool condition, const char* message) {
    if (!condition)
        throw std::runtime_error(message);
}

void frame(LensState& state, double time) {
    // Match PluginRuntime's early return while fully hidden.
    if (state.snapshot().requestedVisible || state.rendering())
        state.advance(time);
}

void settle(LensState& state, double start, double fps) {
    for (int i = 1; i <= static_cast<int>(fps); ++i)
        frame(state, start + i / fps);
}
}

int main() {
    for (const double fps : {30.0, 60.0, 144.0, 240.0}) {
        const double dt = 1.0 / fps;
        for (const bool pinned : {false, true}) {
            LensState state;
            state.setVisible(true, {300.0, 300.0}, 0.0);
            frame(state, dt);
            const auto first = state.snapshot();
            require(first.reveal > 0.0 && first.reveal < 1.0, "First show skipped reveal");
            require(first.wobble > 0.0, "First show lost opening wobble");
            settle(state, dt, fps);
            if (pinned)
                state.setPinned(true, {300.0, 300.0}, 1.1);

            state.setVisible(false, {300.0, 300.0}, 2.0);
            require(state.snapshot().reveal == 1.0, "Hide jumped immediately after an idle gap");
            frame(state, 2.0 + dt);
            require(state.snapshot().reveal > 0.0 && state.snapshot().reveal < 1.0,
                    "Hide must animate after an idle gap");
            settle(state, 2.0 + dt, fps);
            require(!state.rendering() && !state.needsAnimation(), "Hidden state did not settle");
            frame(state, 20.0);
            state.toggle({500.0, 500.0}, 20.0);
            require(state.snapshot().reveal == 0.0, "Reopen jumped before first frame");
            frame(state, 20.0 + dt);
            const auto reopened = state.snapshot();
            require(std::abs(reopened.reveal - first.reveal) < 1e-10,
                    "Idle reopen differs from first opening animation");
            require(std::abs(reopened.wobble - first.wobble) < 1e-10, "Idle reopen lost opening wobble");
            require(length(subtract(reopened.center, pinned ? Vec2{300.0, 300.0} : Vec2{500.0, 500.0})) < 1e-6,
                    "Reopen changed pinned position or failed to follow pointer");

            // Reversing during a fade must account for the time up to the event
            // using the old target, then start from that same visible shape.
            LensState expected = state;
            expected.advance(20.0 + 2.0 * dt);
            state.setVisible(false, reopened.center, 20.0 + 2.0 * dt);
            require(std::abs(state.snapshot().reveal - expected.snapshot().reveal) < 1e-10,
                    "Hide reversal reset reveal or applied old time to new target");
            frame(state, 20.0 + 3.0 * dt);
            require(state.snapshot().reveal < expected.snapshot().reveal, "Hide reversal did not fade out");
            expected = state;
            expected.advance(20.0 + 4.0 * dt);
            state.setVisible(true, reopened.center, 20.0 + 4.0 * dt);
            require(std::abs(state.snapshot().reveal - expected.snapshot().reveal) < 1e-10,
                    "Show reversal reset reveal or applied old time to new target");
            frame(state, 20.0 + 5.0 * dt);
            require(state.snapshot().reveal > expected.snapshot().reveal, "Show reversal did not fade in");
            expected = state;
            expected.advance(20.0 + 6.0 * dt);
            state.setVisible(true, reopened.center, 20.0 + 6.0 * dt);
            require(std::abs(state.snapshot().wobble - expected.snapshot().wobble) < 1e-10,
                    "Repeated show restarted opening wobble");
            state.advance(30.0);
            require(state.snapshot().reveal == 1.0 && state.snapshot().wobble == 0.0,
                    "Long frame stall no longer settles animation");
        }
    }
}
