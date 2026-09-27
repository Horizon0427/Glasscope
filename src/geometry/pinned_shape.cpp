#include "glasscope/geometry/pinned_shape.hpp"
#include <algorithm>
#include <cmath>
namespace Glasscope {
double pinnedDistance(Vec2 point, const PinnedShape& shape) noexcept {
    // Jamie Wong's inverse-square field, converted to a boundary-distance estimate.
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
    thread_local double cachedStrain = -1.0, cachedShare = -1.0;
    thread_local PinnedShape cached;
    if (strain == cachedStrain && share == cachedShare)
        return cached;
    PinnedShape shape;
    shape.bodyRadius = std::sqrt(1.0 - share);
    shape.pullRadius = std::sqrt(share);
    // Stay below the saddle threshold to keep both lobes connected.
    const double critical =
        std::pow(std::pow(shape.bodyRadius, 2.0 / 3.0) + std::pow(shape.pullRadius, 2.0 / 3.0), 1.5);
    const double limit = critical * 0.985;
    const double onset = std::min(strain / 0.12, 1.0);
    const double smoothStrain = strain * onset * (2.0 - onset);
    shape.separation = limit * std::tanh(smoothStrain * 1.6 / limit);
    // Normalize the fused shape, including its neck, to area pi.
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

}
