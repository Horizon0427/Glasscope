#include "glasscope/math/damped_spring.hpp"
#include <cmath>
namespace Glasscope {
void advanceDampedSpring(double& position, double& velocity, double target, double damping, double frequency,
                         double elapsed) {
    const double offset = position - target;
    const double decay = std::exp(-damping * elapsed);
    if (frequency <= 0.0) {
        const double momentum = velocity + damping * offset;
        position = target + decay * (offset + momentum * elapsed);
        velocity = decay * (velocity - damping * momentum * elapsed);
        return;
    }
    const double cosine = std::cos(frequency * elapsed);
    const double sine = std::sin(frequency * elapsed);
    position = target + decay * (offset * cosine + (velocity + damping * offset) / frequency * sine);
    velocity = decay * (velocity * cosine -
                        (damping * velocity + (damping * damping + frequency * frequency) * offset) / frequency * sine);
}
}
