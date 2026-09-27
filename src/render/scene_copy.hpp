#pragma once
#include <GLES3/gl32.h>
#include <string>
namespace Glasscope {
class SceneCopy {
  public:
    SceneCopy() = default;
    SceneCopy(const SceneCopy&) = delete;
    SceneCopy& operator=(const SceneCopy&) = delete;
    bool initialize(std::string& error);
    bool ensure(int width, int height, bool nearest, std::string& error);
    void release();
    void destroy();
    [[nodiscard]] GLuint texture() const {
        return m_copyTexture;
    }
    [[nodiscard]] int width() const {
        return m_copyWidth;
    }
    [[nodiscard]] int height() const {
        return m_copyHeight;
    }

  private:
    GLuint m_copyTexture = 0;
    int m_copyWidth = 0;
    int m_copyHeight = 0;
    int m_maxTextureSize = 0;
    bool m_nearest = false;
};
}
