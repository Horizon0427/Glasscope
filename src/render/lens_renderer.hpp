#pragma once

#include "glasscope/model/color_sample.hpp"
#include "glasscope/model/lens_config.hpp"

#include "render/scene_copy.hpp"
#include "render/shader_program.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace Glasscope {

struct LensRenderParams {
    int framebufferWidth = 0;
    int framebufferHeight = 0;
    float centerX = 0.0F;
    float centerY = 0.0F;
    float scale = 1.0F;
    LensStyle style;
    float timeSeconds = 0.0F;
    float velocityX = 0.0F;
    float velocityY = 0.0F;
    std::array<float, 2> pullAxis = {1.0F, 0.0F};
    std::array<float, 6> trailNodes = {};
    float reveal = 1.0F;
    float wobble = 0.0F;
    float interactionWobble = 0.0F;
    bool pinned = false;
    float pullShare = 0.28F;
    float colorProbeAmount = 0.0F;
    float colorProbeCaptured = 0.0F;
    std::array<float, 3> colorProbeColor = {};
    bool captureColor = false;
};

class LensRenderer {
  public:
    LensRenderer() = default;
    ~LensRenderer() = default;

    LensRenderer(const LensRenderer&) = delete;
    LensRenderer& operator=(const LensRenderer&) = delete;

    bool draw(const LensRenderParams& params);
    [[nodiscard]] std::optional<ColorSample> takeColorSample();
    void releaseCopyTexture();
    void destroy();

    [[nodiscard]] bool failed() const;
    [[nodiscard]] std::optional<std::string> takeError();

  private:
    bool initialize();
    bool checkGlError(const char* operation);
    void fail(std::string message);

    std::array<ShaderProgram, 2> m_shaders;
    GLuint m_vertexArray = 0;
    GLuint m_vertexBuffer = 0;
    SceneCopy m_scene;

    std::optional<ColorSample> m_colorSample;
    std::optional<std::string> m_error;
};

}
