#pragma once
namespace Glasscope {
class CursorOverride {
  public:
    void update(bool aiming);
    void reset();

  private:
    bool m_hidden = false;
};
}
