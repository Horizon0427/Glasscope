#ifdef GLASSCOPE_PLUGIN
in vec2 vUv;
uniform sampler2D uTexture;
out vec4 outColor;
#else
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
out vec4 finalColor;
#define vUv fragTexCoord
#define uTexture texture0
#define outColor finalColor
#endif

uniform vec2  uResolution;
uniform vec2  uTextureMax;
uniform vec2  uCenter;
uniform vec2  uVelocity;
uniform vec2  uPullAxis;
uniform vec3  uPullShape;
uniform vec2  uTrailNodes[3];
uniform float uRadiusPx;
uniform float uZoom;
uniform float uTime;
uniform float uStrength;
uniform float uDispersion;
uniform float uReveal;
uniform float uWobble;
uniform float uInteractionStretch;
uniform float uPinned;
uniform float uMotionStrength;
uniform float uBulge;
uniform float uEdgeWidthPx;
uniform float uEdgeStrength;
uniform float uColorStrength;
uniform float uColorWidthPx;
uniform vec4  uTransmissionColor;
uniform vec4  uRefractionColor;
uniform vec4  uReflectionColor;
uniform vec4  uHighlightColor;
uniform float uScale;
uniform float uColorProbeAmount;
uniform float uColorProbeCaptured;
uniform vec3  uColorProbeColor;

#ifndef GLASSCOPE_BEZIER_SAMPLES
#define GLASSCOPE_BEZIER_SAMPLES 12
#endif

