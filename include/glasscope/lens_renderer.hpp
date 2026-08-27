#pragma once

#include "glasscope/lens_config.hpp"

#include <GLES3/gl32.h>

#include <array>
#include <optional>
#include <string>

namespace Glasscope {

struct LensRenderParams {
    int framebufferWidth = 0;
    int framebufferHeight = 0;
    float centerX = 0.0F; // pixels from the left edge
    float centerY = 0.0F; // pixels from the top edge
    float scale = 1.0F;
    LensStyle style;
    float timeSeconds = 0.0F;
    float velocityX = 0.0F;
    float velocityY = 0.0F;
    std::array<float, 6> trailNodes = {};
    float reveal = 1.0F;
    float wobble = 0.0F;
};

class LensRenderer {
  public:
    LensRenderer() = default;
    ~LensRenderer() = default;

    LensRenderer(const LensRenderer&) = delete;
    LensRenderer& operator=(const LensRenderer&) = delete;

    bool draw(const LensRenderParams& params);
    void releaseCopyTexture();
    void destroy();

    [[nodiscard]] bool failed() const;
    [[nodiscard]] std::optional<std::string> takeError();

  private:
    struct ShaderProgram {
        GLuint program = 0;
        GLint texture = -1;
        GLint resolution = -1;
        GLint textureMax = -1;
        GLint center = -1;
        GLint velocity = -1;
        GLint trail = -1;
        GLint radius = -1;
        GLint zoom = -1;
        GLint time = -1;
        GLint strength = -1;
        GLint dispersion = -1;
        GLint reveal = -1;
        GLint wobble = -1;
        GLint motionStrength = -1;
        GLint bulge = -1;
        GLint edgeWidth = -1;
        GLint edgeStrength = -1;
    };

    bool initialize();
    bool initializeProgram(ShaderProgram& shader, GLuint vertexShader, int bezierSamples);
    bool ensureCopyTextureCapacity(int width, int height, bool nearest);
    GLuint compile(GLenum type, const std::string& source);
    bool checkGlError(const char* operation);
    static void clearGlErrors();
    void fail(std::string message);

    std::array<ShaderProgram, 2> m_shaders;
    GLuint m_vertexArray = 0;
    GLuint m_vertexBuffer = 0;
    GLuint m_copyTexture = 0;
    int m_copyWidth = 0;
    int m_copyHeight = 0;
    int m_maxTextureSize = 0;
    bool m_nearest = false;

    std::optional<std::string> m_error;
};

} // namespace Glasscope
