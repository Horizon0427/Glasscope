#include "glasscope/lens_geometry.hpp"
#include "glasscope/lens_config.hpp"
#include "glasscope/lens_sampling.hpp"
#include "glasscope/lens_state.hpp"

#include <algorithm>

namespace Glasscope {

double pinnedDistance(Vec2 point, const PinnedShape& shape) noexcept {
    // Inverse-square metaballs, following Jamie Wong's field construction.
    // Convert the field to a first-order distance near its boundary so the
    // existing optical rim and hit-test inset remain expressed in pixels.
    const Vec2 pulled = {point.x - shape.separation, point.y};
    const double bodySquared = std::max(point.x * point.x + point.y * point.y, 1e-8);
    const double pullSquared = std::max(pulled.x * pulled.x + pulled.y * pulled.y, 1e-8);
    const double bodyField = shape.bodyRadius * shape.bodyRadius / bodySquared;
    const double pullField = shape.pullRadius * shape.pullRadius / pullSquared;
    const double field = std::max(bodyField + pullField, 1e-12);
    const double inverseRoot = 1.0 / std::sqrt(field);
    const Vec2 gradient = {(bodyField * point.x / bodySquared + pullField * pulled.x / pullSquared),
                           (bodyField * point.y / bodySquared + pullField * pulled.y / pullSquared)};
    const double scale = std::hypot(shape.bodyRadius, shape.pullRadius);
    const double slope = length(gradient) * inverseRoot / field;
    return (inverseRoot - 1.0) / std::max(slope, 0.2 / std::max(scale, 0.001));
}

PinnedShape pinnedShape(double strain, double share) noexcept {
    strain = std::clamp(strain, 0.0, 2.6);
    share = std::clamp(share, 0.06, 0.5);
    if (strain < 0.00001)
        return {};
    // Hit testing samples many points from one snapshot. Reuse the shape integral.
    thread_local double cachedStrain = -1.0, cachedShare = -1.0;
    thread_local PinnedShape cached;
    if (strain == cachedStrain && share == cachedShare)
        return cached;
    PinnedShape shape;
    shape.bodyRadius = std::sqrt(1.0 - share);
    shape.pullRadius = std::sqrt(share);
    // The field's axial saddle reaches the isovalue at this separation.
    // Approach it smoothly with a margin, keeping one connected liquid lens.
    // Starting with coincident sources also makes zero strain exactly circular.
    const double critical =
        std::pow(std::pow(shape.bodyRadius, 2.0 / 3.0) + std::pow(shape.pullRadius, 2.0 / 3.0), 1.5);
    const double limit = critical * 0.985;
    const double onset = std::min(strain / 0.12, 1.0);
    const double smoothStrain = strain * onset * (2.0 - onset);
    shape.separation = limit * std::tanh(smoothStrain * 1.6 / limit);
    // Normalize the complete union, including the fused neck, to the resting area.
    // This runs once per distinct shape on the CPU, never per fragment.
    constexpr int intervals = 96;
    const double left = -1.0;
    const double right = shape.separation + 1.0;
    const double dx = (right - left) / intervals;
    double area = 0.0;
    for (int i = 0; i <= intervals; ++i) {
        const double x = left + i * dx;
        double low = 0.0, high = 1.0;
        for (int j = 0; j < 14; ++j) {
            const double mid = (low + high) * 0.5;
            if (pinnedDistance({x, mid}, shape) <= 0.0)
                low = mid;
            else
                high = mid;
        }
        area += low * (i == 0 || i == intervals ? 1.0 : (i % 2 ? 4.0 : 2.0));
    }
    area *= 2.0 * dx / 3.0;
    const double scale = std::sqrt(std::acos(-1.0) / std::max(area, 0.001));
    shape.bodyRadius *= scale;
    shape.pullRadius *= scale;
    shape.separation *= scale;
    cachedStrain = strain;
    cachedShare = share;
    cached = shape;
    return shape;
}

double lensExtent(const LensGeometry& geometry) noexcept {
    const double trailLength = std::min(geometry.maximumTrailLength * geometry.motionStrength, geometry.radius * 2.6);
    const double padding = 28.0 + geometry.edgeWidth * 0.25 +
                           geometry.edgeStrength * (geometry.refraction * 22.0 + geometry.dispersion * 8.0) +
                           trailLength * 1.10;
    return std::max(geometry.radius + padding, geometry.radius * 1.35 + 4.0);
}

bool lensContains(Vec2 point, const LensSnapshot& snapshot, const LensStyle& style, double scale) noexcept {
    if (!snapshot.requestedVisible || snapshot.reveal < 0.5 || scale <= 0.0)
        return false;
    point = subtract(point, snapshot.center);
    const double motion = std::clamp(static_cast<double>(style.motionStrength), 0.0, 2.5);
    const double wobbleGain =
        snapshot.pinned ? std::clamp(static_cast<double>(style.interactionBounce), 0.0, 2.5) : 1.0;
    const double wobble = std::clamp(snapshot.wobble * wobbleGain, -1.0, 1.0);
    const double radius = style.radius * (0.82 + 0.18 * snapshot.reveal) * (1.0 + wobble * motion * 0.020);
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
        const double stretch = std::exp(
            0.28 * std::tanh(snapshot.interactionWobble *
                             std::clamp(static_cast<double>(style.interactionBounce), 0.0, 2.5) * motion / 1.5));
        const Vec2 local = {(point.x * direction.x + point.y * direction.y) / (radius * stretch),
                            (-point.x * direction.y + point.y * direction.x) * stretch / radius};
        // The affine warp has unit determinant, preserving the lens area.
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
