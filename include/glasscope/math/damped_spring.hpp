#pragma once
namespace Glasscope {
// frequency is damped rad/s; <= 0 selects critical damping.
void advanceDampedSpring(double& position, double& velocity, double target, double damping, double frequency,
                         double elapsed);
}
