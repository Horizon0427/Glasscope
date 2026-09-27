#pragma once
#include <GLES3/gl32.h>
#include <string>
namespace Glasscope {
GLuint compileShader(GLenum type, const std::string& source, std::string& error);
// destroy() requires a current GL context.
struct ShaderProgram {
    ShaderProgram() = default;
    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;
    bool initialize(GLuint vertexShader, int bezierSamples, std::string& error);
    void destroy();
    GLuint program = 0;
    GLint texture = -1;
    GLint resolution = -1;
    GLint textureMax = -1;
    GLint center = -1;
    GLint velocity = -1;
    GLint pullAxis = -1;
    GLint pullShape = -1;
    GLint trail = -1;
    GLint radius = -1;
    GLint zoom = -1;
    GLint time = -1;
    GLint strength = -1;
    GLint dispersion = -1;
    GLint reveal = -1;
    GLint wobble = -1;
    GLint interactionStretch = -1;
    GLint pinned = -1;
    GLint motionStrength = -1;
    GLint bulge = -1;
    GLint edgeWidth = -1;
    GLint edgeStrength = -1;
    GLint colorStrength = -1;
    GLint colorWidth = -1;
    GLint transmissionColor = -1;
    GLint refractionColor = -1;
    GLint reflectionColor = -1;
    GLint highlightColor = -1;
    GLint scale = -1;
    GLint colorProbeAmount = -1;
    GLint colorProbeCaptured = -1;
    GLint colorProbeColor = -1;
};
}
