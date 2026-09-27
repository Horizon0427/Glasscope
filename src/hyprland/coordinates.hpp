#pragma once
#include "glasscope/math/vector.hpp"
#include <hyprutils/math/Vector2D.hpp>
namespace Glasscope {
inline Vec2 fromHypr(Hyprutils::Math::Vector2D value) {
    return {value.x, value.y};
}
inline Hyprutils::Math::Vector2D toHypr(Vec2 value) {
    return {value.x, value.y};
}
}
