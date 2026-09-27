#include "render/shader_program.hpp"
#include "shaders.hpp"
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

}
bool ShaderProgram::initialize(GLuint vertexShader, int bezierSamples, std::string& error) {
    const std::string fragmentSource =
        "#version 320 es\nprecision highp float;\n#define GLASSCOPE_PLUGIN 1\n#define GLASSCOPE_BEZIER_SAMPLES " +
        std::to_string(bezierSamples) + "\n" + std::string(Shaders::FRAGMENT_BODY);
    const GLuint fragment = compileShader(GL_FRAGMENT_SHADER, fragmentSource, error);
    if (fragment == 0)
        return false;

    program = glCreateProgram();
    if (program == 0) {
        glDeleteShader(fragment);
        error = "could not create a shader program object";
        return false;
    }
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glDeleteShader(fragment);

    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        const std::string log = programLog(program);
        glDeleteProgram(program);
        program = 0;
        error = "shader link failed: " + log;
        return false;
    }

    texture = glGetUniformLocation(program, "uTexture");
    resolution = glGetUniformLocation(program, "uResolution");
    textureMax = glGetUniformLocation(program, "uTextureMax");
    center = glGetUniformLocation(program, "uCenter");
    velocity = glGetUniformLocation(program, "uVelocity");
    pullAxis = glGetUniformLocation(program, "uPullAxis");
    pullShape = glGetUniformLocation(program, "uPullShape");
    trail = glGetUniformLocation(program, "uTrailNodes[0]");
    radius = glGetUniformLocation(program, "uRadiusPx");
    zoom = glGetUniformLocation(program, "uZoom");
    time = glGetUniformLocation(program, "uTime");
    strength = glGetUniformLocation(program, "uStrength");
    dispersion = glGetUniformLocation(program, "uDispersion");
    reveal = glGetUniformLocation(program, "uReveal");
    interactionStretch = glGetUniformLocation(program, "uInteractionStretch");
    wobble = glGetUniformLocation(program, "uWobble");
    pinned = glGetUniformLocation(program, "uPinned");
    motionStrength = glGetUniformLocation(program, "uMotionStrength");
    bulge = glGetUniformLocation(program, "uBulge");
    edgeWidth = glGetUniformLocation(program, "uEdgeWidthPx");
    edgeStrength = glGetUniformLocation(program, "uEdgeStrength");
    colorStrength = glGetUniformLocation(program, "uColorStrength");
    colorWidth = glGetUniformLocation(program, "uColorWidthPx");
    transmissionColor = glGetUniformLocation(program, "uTransmissionColor");
    refractionColor = glGetUniformLocation(program, "uRefractionColor");
    reflectionColor = glGetUniformLocation(program, "uReflectionColor");
    highlightColor = glGetUniformLocation(program, "uHighlightColor");
    scale = glGetUniformLocation(program, "uScale");
    colorProbeAmount = glGetUniformLocation(program, "uColorProbeAmount");
    colorProbeCaptured = glGetUniformLocation(program, "uColorProbeCaptured");
    colorProbeColor = glGetUniformLocation(program, "uColorProbeColor");

    if (texture < 0 || resolution < 0 || textureMax < 0 || center < 0 || velocity < 0 || pullAxis < 0 ||
        pullShape < 0 || trail < 0 || radius < 0 || zoom < 0 || time < 0 || strength < 0 || dispersion < 0 ||
        reveal < 0 || wobble < 0 || interactionStretch < 0 || pinned < 0 || motionStrength < 0 || bulge < 0 ||
        edgeWidth < 0 || edgeStrength < 0) {
        error = "shader is missing one or more required uniforms";
        return false;
    }
    if (colorStrength < 0 || colorWidth < 0 || transmissionColor < 0 || refractionColor < 0 || reflectionColor < 0 ||
        highlightColor < 0 || scale < 0 || colorProbeAmount < 0 || colorProbeCaptured < 0 || colorProbeColor < 0) {
        error = "shader is missing one or more required uniforms";
        return false;
    }
    return true;
}

GLuint compileShader(GLenum type, const std::string& source, std::string& error) {
    const GLuint shader = glCreateShader(type);
    if (shader == 0) {
        error = "could not create a shader object";
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
    error = "shader compilation failed: " + log;
    return 0;
}

void ShaderProgram::destroy() {
    if (program != 0)
        glDeleteProgram(program);
    program = 0;
}
}
