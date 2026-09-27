#include "render/lens_renderer.hpp"

#include "glasscope/geometry/lens_geometry.hpp"
#include "glasscope/geometry/lens_sampling.hpp"
#include "render/gl_error.hpp"
#include "render/gl_state_guard.hpp"
#include "shaders.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace Glasscope {

bool LensRenderer::draw(const LensRenderParams& params) {
    if (m_error.has_value())
        return false;
    const float radiusPx = params.style.radius * params.scale;
    const float edgeWidthPx = params.style.edgeWidth * params.scale;
    if (params.framebufferWidth <= 0 || params.framebufferHeight <= 0 || radiusPx <= 1.0F || params.reveal <= 0.0F)
        return true;

    GLStateGuard stateGuard;
    clearGlErrors();
    if (!initialize())
        return false;

    const float extent = static_cast<float>(lensExtent(params.style, radiusPx, edgeWidthPx, params.trailNodes));
    const int desiredLeft = static_cast<int>(std::floor(params.centerX - extent));
    const int desiredRight = static_cast<int>(std::ceil(params.centerX + extent));
    const int desiredTop = static_cast<int>(std::floor(params.centerY - extent));
    const int desiredBottom = static_cast<int>(std::ceil(params.centerY + extent));

    const int left = std::clamp(desiredLeft, 0, params.framebufferWidth);
    const int right = std::clamp(desiredRight, 0, params.framebufferWidth);
    const int top = std::clamp(desiredTop, 0, params.framebufferHeight);
    const int bottom = std::clamp(desiredBottom, 0, params.framebufferHeight);
    const int copyWidth = right - left;
    const int copyHeight = bottom - top;
    if (copyWidth <= 1 || copyHeight <= 1)
        return true;

    std::string resourceError;
    if (!m_scene.ensure(copyWidth, copyHeight, params.style.nearest, resourceError)) {
        fail(std::move(resourceError));
        return false;
    }

    const int sourceY = top;
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_scene.texture());

    GLint drawFramebuffer = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFramebuffer);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(drawFramebuffer));
    glReadBuffer(drawFramebuffer == 0 ? GL_BACK : GL_COLOR_ATTACHMENT0);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, left, sourceY, copyWidth, copyHeight);
    if (!checkGlError("framebuffer copy"))
        return false;

    if (params.captureColor) {
        const int sampleX = std::clamp(static_cast<int>(std::lround(params.centerX)), 0, params.framebufferWidth - 1);
        const int sampleY = std::clamp(static_cast<int>(std::lround(params.centerY)), 0, params.framebufferHeight - 1);
        std::array<std::uint8_t, 4> pixel = {};
        glReadPixels(sampleX, sampleY, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
        if (!checkGlError("color probe readback"))
            return false;
        m_colorSample = ColorSample{.red = pixel[0], .green = pixel[1], .blue = pixel[2]};
    }

    const float textureWidth = static_cast<float>(m_scene.width());
    const float textureHeight = static_cast<float>(m_scene.height());
    const float textureMaxU = static_cast<float>(copyWidth) / textureWidth;
    const float textureMaxV = static_cast<float>(copyHeight) / textureHeight;
    const float centerU = (params.centerX - static_cast<float>(left)) / textureWidth;
    const float centerV = (params.centerY - static_cast<float>(sourceY)) / textureHeight;

    const float x0 = static_cast<float>(left) / static_cast<float>(params.framebufferWidth) * 2.0F - 1.0F;
    const float x1 = static_cast<float>(right) / static_cast<float>(params.framebufferWidth) * 2.0F - 1.0F;
    const float y0 = static_cast<float>(sourceY) / static_cast<float>(params.framebufferHeight) * 2.0F - 1.0F;
    const float y1 =
        static_cast<float>(sourceY + copyHeight) / static_cast<float>(params.framebufferHeight) * 2.0F - 1.0F;

    const std::array<float, 16> vertices = {
        x0, y0, 0.0F, 0.0F, x1, y0, textureMaxU, 0.0F, x0, y1, 0.0F, textureMaxV, x1, y1, textureMaxU, textureMaxV,
    };

    const ShaderProgram& shader = m_shaders[lensBezierSamples(radiusPx) == 10 ? 0 : 1];
    glUseProgram(shader.program);
    glBindVertexArray(m_vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(),
                 GL_STREAM_DRAW);

    glUniform1i(shader.texture, 0);
    glUniform2f(shader.resolution, textureWidth, textureHeight);
    glUniform2f(shader.textureMax, textureMaxU, textureMaxV);
    glUniform2f(shader.center, centerU, centerV);
    glUniform2f(shader.velocity, params.velocityX, params.velocityY);
    glUniform2fv(shader.pullAxis, 1, params.pullAxis.data());
    const float wobbleGain = params.pinned ? std::clamp(params.style.interactionBounce, 0.0F, 2.5F) : 1.0F;
    const float wobble = std::clamp(params.wobble * wobbleGain, -1.0F, 1.0F);
    if (params.pinned) {
        const double motion = std::clamp(static_cast<double>(params.style.motionStrength), 0.0, 2.5);
        const double radius = effectiveLensRadius(radiusPx, params.reveal, wobble, motion);
        const double axisLength = std::max(std::hypot(params.pullAxis[0], params.pullAxis[1]), 0.001F);
        const double strain = (params.trailNodes[4] * params.pullAxis[0] + params.trailNodes[5] * params.pullAxis[1]) *
                              motion / (radius * axisLength);
        const PinnedShape shape = pinnedShape(strain, params.pullShare);
        glUniform3f(shader.pullShape, static_cast<float>(shape.bodyRadius), static_cast<float>(shape.pullRadius),
                    static_cast<float>(shape.separation));
    }
    glUniform2fv(shader.trail, 3, params.trailNodes.data());
    glUniform1f(shader.radius, radiusPx);
    glUniform1f(shader.zoom, std::max(params.style.zoom, LensLimits::ZOOM_MIN));
    glUniform1f(shader.time, params.timeSeconds);
    glUniform1f(shader.strength, std::max(params.style.refraction, LensLimits::REFRACTION_MIN));
    glUniform1f(shader.dispersion, std::max(params.style.dispersion, LensLimits::DISPERSION_MIN));
    glUniform1f(shader.reveal, std::clamp(params.reveal, 0.0F, 1.0F));
    glUniform1f(shader.wobble, wobble);
    glUniform1f(shader.interactionStretch,
                static_cast<float>(interactionStretch(params.interactionWobble, params.style, params.pinned)));
    glUniform1f(shader.pinned, params.pinned ? 1.0F : 0.0F);
    glUniform1f(shader.motionStrength, std::clamp(params.style.motionStrength, LensLimits::MOTION_STRENGTH_MIN,
                                                  LensLimits::MOTION_STRENGTH_MAX));
    glUniform1f(shader.bulge, std::clamp(params.style.bulge, LensLimits::BULGE_MIN, LensLimits::BULGE_MAX));
    glUniform1f(shader.edgeWidth, std::max(edgeWidthPx, LensLimits::EDGE_WIDTH_MIN));
    glUniform1f(shader.edgeStrength, std::max(params.style.edgeStrength, LensLimits::EDGE_STRENGTH_MIN));
    glUniform1f(shader.colorStrength,
                std::clamp(params.style.colorStrength, LensLimits::COLOR_STRENGTH_MIN, LensLimits::COLOR_STRENGTH_MAX));
    glUniform1f(shader.colorWidth, params.style.colorWidth * params.scale);
    glUniform4fv(shader.transmissionColor, 1, params.style.colors.transmission.data());
    glUniform4fv(shader.refractionColor, 1, params.style.colors.refraction.data());
    glUniform4fv(shader.reflectionColor, 1, params.style.colors.reflection.data());
    glUniform4fv(shader.highlightColor, 1, params.style.colors.highlight.data());
    glUniform1f(shader.scale, std::max(params.scale, 0.25F));
    glUniform1f(shader.colorProbeAmount, std::clamp(params.colorProbeAmount, 0.0F, 1.0F));
    glUniform1f(shader.colorProbeCaptured, std::clamp(params.colorProbeCaptured, 0.0F, 1.0F));
    glUniform3fv(shader.colorProbeColor, 1, params.colorProbeColor.data());

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glEnable(GL_SCISSOR_TEST);
    glScissor(left, sourceY, copyWidth, copyHeight);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    return checkGlError("lens draw");
}

std::optional<ColorSample> LensRenderer::takeColorSample() {
    auto result = m_colorSample;
    m_colorSample.reset();
    return result;
}

void LensRenderer::releaseCopyTexture() {
    m_scene.release();
}
void LensRenderer::destroy() {
    m_scene.destroy();
    if (m_vertexBuffer != 0)
        glDeleteBuffers(1, &m_vertexBuffer);
    if (m_vertexArray != 0)
        glDeleteVertexArrays(1, &m_vertexArray);
    for (auto& shader : m_shaders)
        shader.destroy();
    m_vertexBuffer = m_vertexArray = 0;
}
bool LensRenderer::failed() const {
    return m_error.has_value();
}
std::optional<std::string> LensRenderer::takeError() {
    auto result = std::move(m_error);
    m_error.reset();
    return result;
}
bool LensRenderer::checkGlError(const char* operation) {
    std::string error;
    if (Glasscope::checkGlError(operation, error))
        return true;
    fail(std::move(error));
    return false;
}
void LensRenderer::fail(std::string message) {
    if (!m_error)
        m_error = std::move(message);
}
bool LensRenderer::initialize() {
    if (m_shaders[0].program != 0 && m_shaders[1].program != 0)
        return true;

    const std::string vertexSource =
        "#version 320 es\nprecision highp float;\n#define GLASSCOPE_PLUGIN 1\n" + std::string(Shaders::VERTEX_BODY);

    std::string error;
    const GLuint vertex = compileShader(GL_VERTEX_SHADER, vertexSource, error);
    if (vertex == 0) {
        fail(std::move(error));
        return false;
    }
    const bool programsReady = m_shaders[0].initialize(vertex, 10, error) && m_shaders[1].initialize(vertex, 12, error);
    glDeleteShader(vertex);
    if (!programsReady) {
        fail(std::move(error));
        destroy();
        return false;
    }

    glGenVertexArrays(1, &m_vertexArray);
    glGenBuffers(1, &m_vertexBuffer);
    glBindVertexArray(m_vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * static_cast<GLsizei>(sizeof(float)), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * static_cast<GLsizei>(sizeof(float)),
                          reinterpret_cast<const void*>(2 * sizeof(float)));

    const bool sceneReady = m_scene.initialize(error);
    if (!sceneReady && !error.empty())
        fail(error);
    if (!sceneReady || m_vertexArray == 0 || m_vertexBuffer == 0 || !checkGlError("renderer initialization")) {
        if (!m_error.has_value())
            fail("renderer initialization returned incomplete OpenGL resources");
        destroy();
        return false;
    }

    return true;
}

}
