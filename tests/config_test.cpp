#include "glasscope/lens_config.hpp"

#include <limits>
#include <stdexcept>

using namespace Glasscope;

namespace {
void require(bool condition, const char* message) {
    if (!condition)
        throw std::runtime_error(message);
}
struct Option {
    float LensStyle::* member;
    float minimum;
    float maximum;
    float fallback;
};
}

int main() {
    // Expected values are the public configuration contract in README.
    const Option options[] = {
        {&LensStyle::radius, 80.0F, 600.0F, 190.0F},       {&LensStyle::zoom, 1.0F, 6.0F, 2.0F},
        {&LensStyle::refraction, 0.0F, 2.0F, 1.0F},        {&LensStyle::dispersion, 0.0F, 2.0F, 0.7F},
        {&LensStyle::bulge, 0.0F, 0.28F, 0.08F},           {&LensStyle::edgeWidth, 4.0F, 48.0F, 22.0F},
        {&LensStyle::edgeStrength, 0.0F, 2.5F, 1.25F},     {&LensStyle::motionStrength, 0.0F, 2.5F, 1.5F},
        {&LensStyle::interactionBounce, 0.0F, 2.5F, 1.0F}, {&LensStyle::colorStrength, 0.0F, 1.0F, 0.0F},
        {&LensStyle::colorWidth, 4.0F, 48.0F, 18.0F},
    };
    for (const Option& option : options) {
        LensStyle style;
        style.nearest = true;
        style.colors.transmission = {0.1F, 0.2F, 0.3F, 0.4F};
        for (float value : {option.minimum, option.maximum, (option.minimum + option.maximum) * 0.5F}) {
            style.*option.member = value;
            const LensStyle bounded = boundedLensStyle(style);
            require(bounded.*option.member == value, "Valid setting changed");
            require(bounded.nearest && bounded.colors.transmission == style.colors.transmission,
                    "Normalization changed nonnumeric options");
        }
        style.*option.member = -std::numeric_limits<float>::max();
        require(boundedLensStyle(style).*option.member == option.minimum, "Lower range not enforced");
        style.*option.member = std::numeric_limits<float>::max();
        require(boundedLensStyle(style).*option.member == option.maximum, "Upper range not enforced");
        for (float value : {std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(),
                            -std::numeric_limits<float>::infinity()}) {
            style.*option.member = value;
            require(boundedLensStyle(style).*option.member == option.fallback, "Nonfinite setting lacks safe default");
        }
    }
}
