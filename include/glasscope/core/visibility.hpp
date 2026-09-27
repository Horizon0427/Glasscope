#pragma once
namespace Glasscope {
class Visibility {
  public:
    void request(bool visible) {
        m_requested = visible;
    }
    void advance(double elapsed);
    void settle();
    // Snap after dynamics; returns whether fully hidden.
    bool finish();
    [[nodiscard]] bool requested() const {
        return m_requested;
    }
    [[nodiscard]] double value() const {
        return m_value;
    }
    [[nodiscard]] bool rendering() const {
        return m_value > 0.001;
    }
    [[nodiscard]] bool needsAnimation() const;

  private:
    bool m_requested = false;
    double m_value = 0.0;
};
}
