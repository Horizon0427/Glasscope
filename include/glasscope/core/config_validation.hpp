#pragma once
#include "glasscope/model/lens_config.hpp"
namespace Glasscope {
float boundedLensValue(float value, float minimum, float maximum, float fallback);
LensStyle boundedLensStyle(LensStyle style);
}
