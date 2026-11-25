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
uniform vec2  uPillAxis;
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

// Pill shape: the lens is a circle swept along the segment [-uPillAxis, uPillAxis]
// (pixels, relative to the lens centre). Every distance that used to be measured
// from the centre is measured from the nearest point of that segment instead, so
// the rim, trail and jelly effects all follow the stretched shape. With
// uPillAxis = 0 this returns `point` unchanged: the plain round lens.
vec2 pillLocal(vec2 point) {
    float lengthSquared = dot(uPillAxis, uPillAxis);
    if (lengthSquared < 1e-4) {
        return point;
    }
    float along = clamp(dot(point, uPillAxis) / lengthSquared, -1.0, 1.0);
    return point - uPillAxis * along;
}

// Distance to a pill of the given radius whose core segment is centred on the
// origin. Equals the circle distance when uPillAxis is zero.
float pillDistance(vec2 point, float radius) {
    return length(pillLocal(point)) - radius;
}

// Where `point` sits around the pill's outline, as an angle in which one full turn
// around the whole outline is 2 pi. Ripples and wobbles are sin/cos of this angle, so
// they run smoothly along the long sides and round the caps as ONE surface instead of
// the caps behaving like a small circle and the sides like something else. For a
// circle this is exactly the polar angle. Measured from the pill's axis.
float pillOutlineAngle(vec2 point, float radius) {
    float halfLength = length(uPillAxis);
    vec2 axis = uPillAxis / halfLength;
    vec2 normal = vec2(-axis.y, axis.x);
    float t = dot(point, axis);
    float u = dot(point, normal);
    const float PI = 3.14159265;
    float perimeter = 2.0 * PI * radius + 4.0 * halfLength;
    float s;
    if (t > halfLength) {
        s = radius * atan(u, t - halfLength);
    } else if (t < -halfLength) {
        float psi = atan(u, t + halfLength);
        if (psi < 0.0) {
            psi += 2.0 * PI;
        }
        s = radius * PI * 0.5 + 2.0 * halfLength + radius * (psi - PI * 0.5);
    } else if (u >= 0.0) {
        s = radius * PI * 0.5 + (halfLength - t);
    } else {
        s = radius * PI * 0.5 + 2.0 * halfLength + radius * PI + (t + halfLength);
    }
    return 2.0 * PI * s / perimeter;
}
