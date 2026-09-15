#include "glasscope/lens_renderer.hpp"

#include "shaders.hpp"

#include <algorithm>
#include <limits>
#include <string>
#include <vector>

namespace Glasscope {
namespace {

std::string shaderLog(GLuint shader) {
    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    if (length <= 1)
        return "unknown shader compilation failure";

    std::vector<char> log(static_cast<std::size_t>(length));
    glGetShaderInfoLog(shader, length, nullptr, log.data());
    return std::string(log.data());
}

std::string programLog(GLuint program) {
    GLint length = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    if (length <= 1)
        return "unknown shader link failure";

    std::vector<char> log(static_cast<std::size_t>(length));
    glGetProgramInfoLog(program, length, nullptr, log.data());
    return std::string(log.data());
}

const char* glErrorName(GLenum error) {
    switch (error) {
        case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
        case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
        case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
        case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
        case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
        default: return "unknown OpenGL error";
    }
}

constexpr int COPY_TEXTURE_BUCKET = 128;

int bucketedTextureExtent(int required) {
    if (required <= 0 || required > std::numeric_limits<int>::max() - (COPY_TEXTURE_BUCKET - 1))
        return 0;
    return ((required + COPY_TEXTURE_BUCKET - 1) / COPY_TEXTURE_BUCKET) * COPY_TEXTURE_BUCKET;
}

}

void LensRenderer::releaseCopyTexture() {
    if (m_copyTexture != 0)
        glDeleteTextures(1, &m_copyTexture);
    m_copyTexture = 0;
    m_copyWidth = 0;
    m_copyHeight = 0;
}

void LensRenderer::destroy() {
    releaseCopyTexture();
    if (m_vertexBuffer != 0)
        glDeleteBuffers(1, &m_vertexBuffer);
    if (m_vertexArray != 0)
        glDeleteVertexArrays(1, &m_vertexArray);
    for (ShaderProgram& shader : m_shaders) {
        if (shader.program != 0)
            glDeleteProgram(shader.program);
        shader = {};
    }

    m_vertexBuffer = 0;
    m_vertexArray = 0;
    m_maxTextureSize = 0;
}

bool LensRenderer::failed() const {
    return m_error.has_value();
}

std::optional<std::string> LensRenderer::takeError() {
    auto result = std::move(m_error);
    m_error.reset();
    return result;
}

bool LensRenderer::initialize() {
    if (m_shaders[0].program != 0 && m_shaders[1].program != 0)
        return true;

    const std::string vertexSource =
        "#version 320 es\nprecision highp float;\n#define GLASSCOPE_PLUGIN 1\n" + std::string(Shaders::VERTEX_BODY);

    const GLuint vertex = compile(GL_VERTEX_SHADER, vertexSource);
    if (vertex == 0)
        return false;
    const bool programsReady = initializeProgram(m_shaders[0], vertex, 10) &&
                               initializeProgram(m_shaders[1], vertex, 12);
    glDeleteShader(vertex);
    if (!programsReady) {
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

    glGenTextures(1, &m_copyTexture);
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &m_maxTextureSize);

    if (m_copyTexture == 0 || m_vertexArray == 0 || m_vertexBuffer == 0 || m_maxTextureSize <= 0 ||
        !checkGlError("renderer initialization")) {
        if (!m_error.has_value())
            fail("renderer initialization returned incomplete OpenGL resources");
        destroy();
        return false;
    }

    return true;
}

bool LensRenderer::initializeProgram(ShaderProgram& shader, GLuint vertexShader, int bezierSamples) {
    const std::string fragmentSource =
        "#version 320 es\nprecision highp float;\n#define GLASSCOPE_PLUGIN 1\n#define GLASSCOPE_BEZIER_SAMPLES " +
        std::to_string(bezierSamples) + "\n" + std::string(Shaders::FRAGMENT_BODY);
    const GLuint fragment = compile(GL_FRAGMENT_SHADER, fragmentSource);
    if (fragment == 0)
        return false;

    shader.program = glCreateProgram();
    if (shader.program == 0) {
        glDeleteShader(fragment);
        fail("could not create a shader program object");
        return false;
    }
    glAttachShader(shader.program, vertexShader);
    glAttachShader(shader.program, fragment);
    glLinkProgram(shader.program);
    glDeleteShader(fragment);

    GLint linked = GL_FALSE;
    glGetProgramiv(shader.program, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        const std::string log = programLog(shader.program);
        glDeleteProgram(shader.program);
        shader.program = 0;
        fail("shader link failed: " + log);
        return false;
    }

    shader.texture = glGetUniformLocation(shader.program, "uTexture");
    shader.resolution = glGetUniformLocation(shader.program, "uResolution");
    shader.textureMax = glGetUniformLocation(shader.program, "uTextureMax");
    shader.center = glGetUniformLocation(shader.program, "uCenter");
    shader.velocity = glGetUniformLocation(shader.program, "uVelocity");
    shader.trail = glGetUniformLocation(shader.program, "uTrailNodes[0]");
    shader.radius = glGetUniformLocation(shader.program, "uRadiusPx");
    shader.zoom = glGetUniformLocation(shader.program, "uZoom");
    shader.time = glGetUniformLocation(shader.program, "uTime");
    shader.strength = glGetUniformLocation(shader.program, "uStrength");
    shader.dispersion = glGetUniformLocation(shader.program, "uDispersion");
    shader.reveal = glGetUniformLocation(shader.program, "uReveal");
    shader.wobble = glGetUniformLocation(shader.program, "uWobble");
    shader.motionStrength = glGetUniformLocation(shader.program, "uMotionStrength");
    shader.bulge = glGetUniformLocation(shader.program, "uBulge");
    shader.edgeWidth = glGetUniformLocation(shader.program, "uEdgeWidthPx");
    shader.edgeStrength = glGetUniformLocation(shader.program, "uEdgeStrength");
    shader.colorStrength = glGetUniformLocation(shader.program, "uColorStrength");
    shader.colorWidth = glGetUniformLocation(shader.program, "uColorWidthPx");
    shader.transmissionColor = glGetUniformLocation(shader.program, "uTransmissionColor");
    shader.refractionColor = glGetUniformLocation(shader.program, "uRefractionColor");
    shader.reflectionColor = glGetUniformLocation(shader.program, "uReflectionColor");
    shader.highlightColor = glGetUniformLocation(shader.program, "uHighlightColor");
    shader.scale = glGetUniformLocation(shader.program, "uScale");
    shader.colorProbeAmount = glGetUniformLocation(shader.program, "uColorProbeAmount");
    shader.colorProbeCaptured = glGetUniformLocation(shader.program, "uColorProbeCaptured");
    shader.colorProbeColor = glGetUniformLocation(shader.program, "uColorProbeColor");

    if (shader.texture < 0 || shader.resolution < 0 || shader.textureMax < 0 || shader.center < 0 ||
        shader.velocity < 0 || shader.trail < 0 || shader.radius < 0 || shader.zoom < 0 || shader.time < 0 ||
        shader.strength < 0 || shader.dispersion < 0 || shader.reveal < 0 || shader.wobble < 0 ||
        shader.motionStrength < 0 || shader.bulge < 0 || shader.edgeWidth < 0 || shader.edgeStrength < 0) {
        fail("shader is missing one or more required uniforms");
        return false;
    }
    if (shader.colorStrength < 0 || shader.colorWidth < 0 || shader.transmissionColor < 0 || shader.refractionColor < 0 || shader.reflectionColor < 0 ||
        shader.highlightColor < 0 || shader.scale < 0 || shader.colorProbeAmount < 0 || shader.colorProbeCaptured < 0 ||
        shader.colorProbeColor < 0) {
        fail("shader is missing one or more required uniforms");
        return false;
    }
    return true;
}

bool LensRenderer::ensureCopyTextureCapacity(int width, int height, bool nearest) {
    const int bucketedWidth = bucketedTextureExtent(width);
    const int bucketedHeight = bucketedTextureExtent(height);
    if (bucketedWidth == 0 || bucketedHeight == 0 || bucketedWidth > m_maxTextureSize ||
        bucketedHeight > m_maxTextureSize) {
        fail("copy texture exceeds the renderer texture-size limit");
        return false;
    }

    if (m_copyTexture == 0) {
        glGenTextures(1, &m_copyTexture);
        if (m_copyTexture == 0 || !checkGlError("copy texture creation"))
            return false;
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_copyTexture);
    const GLint filter = nearest ? GL_NEAREST : GL_LINEAR;
    const int capacityWidth = std::max(m_copyWidth, bucketedWidth);
    const int capacityHeight = std::max(m_copyHeight, bucketedHeight);
    const bool grown = capacityWidth != m_copyWidth || capacityHeight != m_copyHeight;
    if (grown) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, capacityWidth, capacityHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        if (!checkGlError("copy texture allocation")) {
            releaseCopyTexture();
            return false;
        }
        m_copyWidth = capacityWidth;
        m_copyHeight = capacityHeight;
    }
    if (m_nearest != nearest || grown) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        m_nearest = nearest;
    }
    return checkGlError("copy texture parameters");
}

GLuint LensRenderer::compile(GLenum type, const std::string& source) {
    const GLuint shader = glCreateShader(type);
    if (shader == 0) {
        fail("could not create a shader object");
        return 0;
    }
    const char* data = source.c_str();
    glShaderSource(shader, 1, &data, nullptr);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_TRUE)
        return shader;

    const std::string log = shaderLog(shader);
    glDeleteShader(shader);
    fail("shader compilation failed: " + log);
    return 0;
}

bool LensRenderer::checkGlError(const char* operation) {
    const GLenum error = glGetError();
    if (error == GL_NO_ERROR)
        return true;

    clearGlErrors();
    fail(std::string(operation) + " failed: " + glErrorName(error));
    return false;
}

void LensRenderer::clearGlErrors() {
    for (int index = 0; index < 32 && glGetError() != GL_NO_ERROR; ++index) {
    }
}

void LensRenderer::fail(std::string message) {
    if (!m_error.has_value())
        m_error = std::move(message);
}

}
