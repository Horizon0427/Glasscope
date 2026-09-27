#include "render/scene_copy.hpp"
#include "render/gl_error.hpp"
#include <algorithm>
#include <limits>
namespace Glasscope {
namespace {
constexpr int COPY_TEXTURE_BUCKET = 128;

int bucketedTextureExtent(int required) {
    if (required <= 0 || required > std::numeric_limits<int>::max() - (COPY_TEXTURE_BUCKET - 1))
        return 0;
    return ((required + COPY_TEXTURE_BUCKET - 1) / COPY_TEXTURE_BUCKET) * COPY_TEXTURE_BUCKET;
}
}
void SceneCopy::release() {
    if (m_copyTexture != 0)
        glDeleteTextures(1, &m_copyTexture);
    m_copyTexture = 0;
    m_copyWidth = 0;
    m_copyHeight = 0;
}

void SceneCopy::destroy() {
    release();
    m_maxTextureSize = 0;
}
bool SceneCopy::initialize(std::string& error) {
    glGenTextures(1, &m_copyTexture);
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &m_maxTextureSize);
    if (m_copyTexture == 0 || m_maxTextureSize <= 0) {
        error = "renderer initialization returned incomplete OpenGL resources";
        return false;
    }
    return true;
}
bool SceneCopy::ensure(int width, int height, bool nearest, std::string& error) {
    const int bucketedWidth = bucketedTextureExtent(width);
    const int bucketedHeight = bucketedTextureExtent(height);
    if (bucketedWidth == 0 || bucketedHeight == 0 || bucketedWidth > m_maxTextureSize ||
        bucketedHeight > m_maxTextureSize) {
        error = "copy texture exceeds the renderer texture-size limit";
        return false;
    }

    if (m_copyTexture == 0) {
        glGenTextures(1, &m_copyTexture);
        if (m_copyTexture == 0 || !checkGlError("copy texture creation", error))
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
        if (!checkGlError("copy texture allocation", error)) {
            release();
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
    return checkGlError("copy texture parameters", error);
}

}
