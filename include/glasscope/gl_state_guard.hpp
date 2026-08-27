#pragma once

#include <GLES3/gl32.h>

#include <array>

namespace Glasscope {

class GLStateGuard {
  public:
    GLStateGuard() {
        glGetIntegerv(GL_CURRENT_PROGRAM, &m_program);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &m_activeTexture);
        glActiveTexture(GL_TEXTURE0);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &m_texture0);
        glActiveTexture(static_cast<GLenum>(m_activeTexture));
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &m_vertexArray);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &m_arrayBuffer);
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &m_readFramebuffer);
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &m_drawFramebuffer);
        glGetIntegerv(GL_READ_BUFFER, &m_readBuffer);
        glGetIntegerv(GL_VIEWPORT, m_viewport.data());
        glGetIntegerv(GL_SCISSOR_BOX, m_scissorBox.data());
        glGetIntegerv(GL_BLEND_SRC_RGB, &m_blendSrcRgb);
        glGetIntegerv(GL_BLEND_DST_RGB, &m_blendDstRgb);
        glGetIntegerv(GL_BLEND_SRC_ALPHA, &m_blendSrcAlpha);
        glGetIntegerv(GL_BLEND_DST_ALPHA, &m_blendDstAlpha);
        glGetIntegerv(GL_BLEND_EQUATION_RGB, &m_blendEquationRgb);
        glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &m_blendEquationAlpha);
        glGetBooleanv(GL_COLOR_WRITEMASK, m_colorMask.data());
        m_blendEnabled = glIsEnabled(GL_BLEND);
        m_scissorEnabled = glIsEnabled(GL_SCISSOR_TEST);
        m_depthEnabled = glIsEnabled(GL_DEPTH_TEST);
        m_cullEnabled = glIsEnabled(GL_CULL_FACE);
    }

    ~GLStateGuard() {
        glUseProgram(static_cast<GLuint>(m_program));
        glBindVertexArray(static_cast<GLuint>(m_vertexArray));
        glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(m_arrayBuffer));
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(m_drawFramebuffer));
        glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(m_readFramebuffer));
        glReadBuffer(static_cast<GLenum>(m_readBuffer));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(m_texture0));
        glActiveTexture(static_cast<GLenum>(m_activeTexture));
        glViewport(m_viewport[0], m_viewport[1], m_viewport[2], m_viewport[3]);
        glScissor(m_scissorBox[0], m_scissorBox[1], m_scissorBox[2], m_scissorBox[3]);
        glBlendFuncSeparate(static_cast<GLenum>(m_blendSrcRgb), static_cast<GLenum>(m_blendDstRgb),
                            static_cast<GLenum>(m_blendSrcAlpha), static_cast<GLenum>(m_blendDstAlpha));
        glBlendEquationSeparate(static_cast<GLenum>(m_blendEquationRgb), static_cast<GLenum>(m_blendEquationAlpha));
        glColorMask(m_colorMask[0], m_colorMask[1], m_colorMask[2], m_colorMask[3]);
        setEnabled(GL_BLEND, m_blendEnabled);
        setEnabled(GL_SCISSOR_TEST, m_scissorEnabled);
        setEnabled(GL_DEPTH_TEST, m_depthEnabled);
        setEnabled(GL_CULL_FACE, m_cullEnabled);
    }

    GLStateGuard(const GLStateGuard&) = delete;
    GLStateGuard& operator=(const GLStateGuard&) = delete;

  private:
    static void setEnabled(GLenum capability, GLboolean enabled) {
        if (enabled == GL_TRUE)
            glEnable(capability);
        else
            glDisable(capability);
    }

    GLint m_program = 0;
    GLint m_activeTexture = 0;
    GLint m_texture0 = 0;
    GLint m_vertexArray = 0;
    GLint m_arrayBuffer = 0;
    GLint m_readFramebuffer = 0;
    GLint m_drawFramebuffer = 0;
    GLint m_readBuffer = GL_BACK;
    std::array<GLint, 4> m_viewport = {};
    std::array<GLint, 4> m_scissorBox = {};
    GLint m_blendSrcRgb = 0;
    GLint m_blendDstRgb = 0;
    GLint m_blendSrcAlpha = 0;
    GLint m_blendDstAlpha = 0;
    GLint m_blendEquationRgb = 0;
    GLint m_blendEquationAlpha = 0;
    std::array<GLboolean, 4> m_colorMask = {};
    GLboolean m_blendEnabled = GL_FALSE;
    GLboolean m_scissorEnabled = GL_FALSE;
    GLboolean m_depthEnabled = GL_FALSE;
    GLboolean m_cullEnabled = GL_FALSE;
};

} // namespace Glasscope
