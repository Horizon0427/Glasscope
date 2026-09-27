#include "render/gl_error.hpp"
#include <GLES3/gl32.h>
namespace Glasscope {
namespace {
const char* glErrorName(GLenum error) {
    switch (error) {
    case GL_INVALID_ENUM:
        return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:
        return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:
        return "GL_INVALID_OPERATION";
    case GL_OUT_OF_MEMORY:
        return "GL_OUT_OF_MEMORY";
    case GL_INVALID_FRAMEBUFFER_OPERATION:
        return "GL_INVALID_FRAMEBUFFER_OPERATION";
    default:
        return "unknown OpenGL error";
    }
}

}
void clearGlErrors() {
    for (int index = 0; index < 32 && glGetError() != GL_NO_ERROR; ++index) {
    }
}
bool checkGlError(const char* operation, std::string& message) {
    const GLenum error = glGetError();
    if (error == GL_NO_ERROR)
        return true;
    clearGlErrors();
    message = std::string(operation) + " failed: " + glErrorName(error);
    return false;
}
}
