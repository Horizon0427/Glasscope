#include "glasscope/geometry/lens_geometry.hpp"
#include "glasscope/core/lens_state.hpp"
#include "glasscope/geometry/lens_sampling.hpp"
#include "glasscope/model/lens_config.hpp"

#include <algorithm>

namespace Glasscope {

namespace {
template <typename T> double stretchFor(T amount, const LensStyle& style, bool pinned) noexcept {
    const double interaction = pinned ? amount * std::clamp(static_cast<T>(style.interactionBounce), T{0}, T{2.5}) *
                                            std::clamp(static_cast<T>(style.motionStrength), T{0}, T{2.5}) / 1.5
                                      : 0.0;
    return std::exp(0.28 * std::tanh(interaction));
}
}
double interactionStretch(double amount, const LensStyle& style, bool pinned) noexcept {
    return stretchFor(amount, style, pinned);
}
double interactionStretch(float amount, const LensStyle& style, bool pinned) noexcept {
    return stretchFor(amount, style, pinned);
}
double effectiveLensRadius(double radius, double reveal, double wobble, double motion) noexcept {
    return radius * (0.82 + 0.18 * reveal) * (1.0 + wobble * motion * 0.020);
}

bool lensContains(Vec2 point, const LensSnapshot& snapshot, const LensStyle& style, double scale) noexcept {
    if (!snapshot.requestedVisible || snapshot.reveal < 0.5 || scale <= 0.0)
        return false;
    point = subtract(point, snapshot.center);
    const double motion = std::clamp(static_cast<double>(style.motionStrength), 0.0, 2.5);
    const double wobbleGain =
        snapshot.pinned ? std::clamp(static_cast<double>(style.interactionBounce), 0.0, 2.5) : 1.0;
    const double wobble = std::clamp(snapshot.wobble * wobbleGain, -1.0, 1.0);
    const double radius = effectiveLensRadius(style.radius, snapshot.reveal, wobble, motion);
    if (radius <= 0.0)
        return false;
    std::array<Vec2, 4> nodes = {};
    double trail = 0.0;
    for (std::size_t i = 1; i < nodes.size(); ++i) {
        nodes[i] = multiply(snapshot.trailNodes[i - 1], motion);
        trail = std::max(trail, length(nodes[i]));
    }
    const double maximumTrail = radius * (snapshot.pinned ? 2.6 : 1.35);
    if (trail > maximumTrail) {
        for (Vec2& node : nodes)
            node = multiply(node, maximumTrail / trail);
        trail = maximumTrail;
    }
    if (length(point) > trail + radius * 1.40 + 2.0 / scale)
        return false;
    const Vec2 velocity = {std::clamp(snapshot.velocity.x / 1800.0, -1.0, 1.0),
                           std::clamp(snapshot.velocity.y / 1800.0, -1.0, 1.0)};
    const double speed = length(velocity);
    Vec2 direction = length(nodes[1]) > 0.5 / scale ? multiply(nodes[1], -1.0 / length(nodes[1]))
                     : speed > 0.0001               ? multiply(velocity, 1.0 / speed)
                                                    : Vec2{1.0, 0.0};
    if (snapshot.pinned)
        direction = multiply(snapshot.pullAxis, 1.0 / std::max(length(snapshot.pullAxis), 0.001));
    const double activation = 1.0 - std::exp(-trail / radius * 3.8);
    double distance = length(point) - radius;
    if (snapshot.pinned) {
        const double strain = (nodes[3].x * direction.x + nodes[3].y * direction.y) / radius;
        const PinnedShape shape = pinnedShape(strain, snapshot.pullShare);
        const double stretch = interactionStretch(snapshot.interactionWobble, style, true);
        const Vec2 local = {(point.x * direction.x + point.y * direction.y) / (radius * stretch),
                            (-point.x * direction.y + point.y * direction.x) * stretch / radius};
        // Unit-determinant warp preserves area.
        distance = radius * pinnedDistance(local, shape) * std::min(stretch, 1.0 / stretch);
    } else if (!snapshot.pinned && activation > 0.0005) {
        double slime = 100000.0;
        const int samples = lensBezierSamples(style.radius * static_cast<float>(scale));
        for (int i = 0; i < samples; ++i) {
            const double t = static_cast<double>(i) / (samples - 1);
            const double v = 1.0 - t;
            const Vec2 q = {nodes[1].x * 3.0 * v * v * t + nodes[2].x * 3.0 * v * t * t + nodes[3].x * t * t * t,
                            nodes[1].y * 3.0 * v * v * t + nodes[2].y * 3.0 * v * t * t + nodes[3].y * t * t * t};
            const Vec2 derivative = {nodes[1].x * 3.0 * v * v + (nodes[2].x - nodes[1].x) * 6.0 * v * t +
                                         (nodes[3].x - nodes[2].x) * 3.0 * t * t,
                                     nodes[1].y * 3.0 * v * v + (nodes[2].y - nodes[1].y) * 6.0 * v * t +
                                         (nodes[3].y - nodes[2].y) * 3.0 * t * t};
            const double materialRadius =
                radius * std::clamp(0.22 + 0.76 / (1.0 + length(derivative) / (radius * 1.45) * 0.98), 0.22, 0.96);
            const double drop = length(subtract(point, q)) - materialRadius;
            const double blend = std::clamp(0.5 + 0.5 * (drop - slime) / (radius * 0.065), 0.0, 1.0);
            slime = drop * (1.0 - blend) + slime * blend - radius * 0.065 * blend * (1.0 - blend);
        }
        distance = distance * (1.0 - activation) + slime * activation;
    }
    const double along = point.x * direction.x + point.y * direction.y;
    const double across = -point.x * direction.y + point.y * direction.x;
    const double angle = std::atan2(across, along);
    const double energy = std::clamp(
        std::clamp(speed * motion, 0.0, 1.18) + activation * 0.28 + std::abs(wobble) * motion * 0.34, 0.0, 1.35);
    const double ripple = std::sin(angle * 3.0 + along / radius * 1.7 - snapshot.phase * 5.2) * 0.008 +
                          std::sin(angle * 5.0 + snapshot.phase * 3.8) * 0.004;
    distance -= radius * ripple * energy;
    distance -= radius * wobble * motion * (std::cos(angle * 2.0) * 0.033 + std::sin(angle * 3.0 + 0.65) * 0.012);
    return distance <= -1.0 / scale;
}

}
