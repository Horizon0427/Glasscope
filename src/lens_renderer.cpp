#include "glasscope/lens_renderer.hpp"

#include "glasscope/gl_state_guard.hpp"
#include "glasscope/lens_geometry.hpp"
#include "glasscope/lens_sampling.hpp"

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

    float maximumTrailLength = 0.0F;
    for (std::size_t index = 0; index < params.trailNodes.size(); index += 2)
        maximumTrailLength =
            std::max(maximumTrailLength, std::hypot(params.trailNodes[index], params.trailNodes[index + 1]));
    const float extent = static_cast<float>(lensExtent({
        .radius = radiusPx,
        .edgeWidth = edgeWidthPx,
        .edgeStrength = params.style.edgeStrength,
        .refraction = params.style.refraction,
        .dispersion = params.style.dispersion,
        .motionStrength = params.style.motionStrength,
        .maximumTrailLength = maximumTrailLength,
    }));
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

    if (!ensureCopyTextureCapacity(copyWidth, copyHeight, params.style.nearest))
        return false;

    const int sourceY = top;
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_copyTexture);

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

    const float textureWidth = static_cast<float>(m_copyWidth);
    const float textureHeight = static_cast<float>(m_copyHeight);
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
        const double radius =
            radiusPx * (0.82 + 0.18 * params.reveal) * (1.0 + static_cast<double>(wobble) * motion * 0.020);
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
    const double interaction = params.pinned
                                   ? params.interactionWobble * std::clamp(params.style.interactionBounce, 0.0F, 2.5F) *
                                         std::clamp(params.style.motionStrength, 0.0F, 2.5F) / 1.5
                                   : 0.0;
    glUniform1f(shader.interactionStretch, static_cast<float>(std::exp(0.28 * std::tanh(interaction))));
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

}
