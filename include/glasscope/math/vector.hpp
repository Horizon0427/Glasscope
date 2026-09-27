#pragma once

#include <cmath>

namespace Glasscope {

struct Vec2 {
    double x = 0.0;
    double y = 0.0;
};

[[nodiscard]] inline double length(Vec2 value) noexcept {
    return std::hypot(value.x, value.y);
}

[[nodiscard]] inline Vec2 multiply(Vec2 value, double factor) noexcept {
    return {value.x * factor, value.y * factor};
}

[[nodiscard]] inline Vec2 subtract(Vec2 lhs, Vec2 rhs) noexcept {
    return {lhs.x - rhs.x, lhs.y - rhs.y};
}

[[nodiscard]] inline Vec2 mix(Vec2 from, Vec2 to, double amount) noexcept {
    return {
        from.x + (to.x - from.x) * amount,
        from.y + (to.y - from.y) * amount,
    };
}

[[nodiscard]] inline Vec2 clampLength(Vec2 value, double maximum) noexcept {
    const double magnitude = length(value);
    if (magnitude <= maximum || magnitude <= 0.0001)
        return value;
    return multiply(value, maximum / magnitude);
}

}
