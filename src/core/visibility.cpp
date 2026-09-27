#include "glasscope/core/visibility.hpp"
#include <cmath>
namespace Glasscope {
void Visibility::advance(double elapsed) {
    const double target = m_requested ? 1.0 : 0.0;
    const double rate = m_requested ? 18.0 : 23.0;
    m_value += (target - m_value) * (1.0 - std::exp(-elapsed * rate));
}
void Visibility::settle() {
    m_value = m_requested ? 1.0 : 0.0;
}
bool Visibility::finish() {
    if (!m_requested && m_value < 0.002) {
        m_value = 0.0;
        return true;
    }
    if (m_requested && m_value > 0.998)
        m_value = 1.0;
    return false;
}
bool Visibility::needsAnimation() const {
    return std::abs((m_requested ? 1.0 : 0.0) - m_value) > 0.002;
}
}
