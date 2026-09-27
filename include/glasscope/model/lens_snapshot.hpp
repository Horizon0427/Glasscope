#pragma once

#include "glasscope/math/vector.hpp"
#include <array>

namespace Glasscope {

struct LensSnapshot {
    Vec2 center;
    Vec2 velocity;
    Vec2 pullAxis = {1.0, 0.0};
    std::array<Vec2, 3> trailNodes;
    double pullShare = 0.28;
    double reveal = 0.0;
    double wobble = 0.0;
    double interactionWobble = 0.0;
    double phase = 0.0;
    bool requestedVisible = false;
    bool pinned = false;
};

}
