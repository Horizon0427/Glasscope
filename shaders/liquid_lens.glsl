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

float smoothMinimum(float a, float b, float radius) {
    float blend = clamp(0.5 + 0.5 * (b - a) / radius, 0.0, 1.0);
    return mix(b, a, blend) - radius * blend * (1.0 - blend);
}

float circleDistance(vec2 point, vec2 center, float radius) {
    return length(point - center) - radius;
}

vec2 cubicBezierPoint(vec2 control[4], float amount) {
    float inverse = 1.0 - amount;
    return control[0] * inverse * inverse * inverse +
           control[1] * 3.0 * inverse * inverse * amount +
           control[2] * 3.0 * inverse * amount * amount +
           control[3] * amount * amount * amount;
}

vec2 cubicBezierDerivative(vec2 control[4], float amount) {
    float inverse = 1.0 - amount;
    return (control[1] - control[0]) * 3.0 * inverse * inverse +
           (control[2] - control[1]) * 6.0 * inverse * amount +
           (control[3] - control[2]) * 3.0 * amount * amount;
}

float perceptualLuminance(vec3 color) {
    return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

vec3 preserveLuminance(vec3 color, float targetLuminance) {
    float difference = targetLuminance - perceptualLuminance(color);
    return clamp(color + vec3(difference), 0.0, 1.0);
}

float roundedBoxDistance(vec2 point, vec2 halfSize, float radius) {
    vec2 offset = abs(point) - halfSize + vec2(radius);
    return min(max(offset.x, offset.y), 0.0) +
           length(max(offset, vec2(0.0))) - radius;
}

void prepareLens(out float motionStrength, out float rawSpeed,
                 out float signedBounce, out float radius,
                 out vec2 nodes[4], out float maximumTrailLength) {
    motionStrength = clamp(uMotionStrength, 0.0, 2.5);
    rawSpeed = length(uVelocity);
    signedBounce = clamp(uWobble, -1.0, 1.0);
    float bounceScale = 1.0 + signedBounce * motionStrength * 0.020;
    radius = uRadiusPx * mix(0.82, 1.0, uReveal) * bounceScale;

    nodes[0] = vec2(0.0);
    nodes[1] = uTrailNodes[0] * motionStrength;
    nodes[2] = uTrailNodes[1] * motionStrength;
    nodes[3] = uTrailNodes[2] * motionStrength;

    maximumTrailLength = max(length(nodes[1]),
                             max(length(nodes[2]), length(nodes[3])));
    float maximumAllowedLength = radius * (uPinned > 0.5 ? 2.6 : 1.35);
    if (maximumTrailLength > maximumAllowedLength) {
        float trailScale = maximumAllowedLength / maximumTrailLength;
        nodes[1] *= trailScale;
        nodes[2] *= trailScale;
        nodes[3] *= trailScale;
        maximumTrailLength = maximumAllowedLength;
    }
}

float lensDistance(vec2 point, float motionStrength, float rawSpeed,
                   float signedBounce, float radius, vec2 nodes[4],
                   float maximumTrailLength) {
    float speed = clamp(rawSpeed * motionStrength, 0.0, 1.18);

    float normalizedTrail = maximumTrailLength / max(radius, 1.0);
    float trailActivation = 1.0 - exp(-normalizedTrail * 3.8);
    vec2 direction = length(nodes[1]) > 0.5 ? normalize(-nodes[1]) :
                     (rawSpeed > 0.0001 ? normalize(uVelocity) :
                                          vec2(1.0, 0.0));
    vec2 perpendicular = vec2(-direction.y, direction.x);
    if (uPinned > 0.5) {
        direction = uPullAxis / max(length(uPullAxis), 0.001);
        perpendicular = vec2(-direction.y, direction.x);
    }
    float along = dot(point, direction);
    float across = dot(point, perpendicular);
    float restDrop = circleDistance(point, vec2(0.0), radius);

    float distance = restDrop;
    if (uPinned > 0.5) {
        float interactionStretch = max(uInteractionStretch, 0.001);
        vec2 local = vec2(along / interactionStretch, across * interactionStretch) / radius;
        // Keep the field-to-distance conversion identical to pinnedDistance().
        vec2 pulled = local - vec2(uPullShape.z, 0.0);
        float bodySquared = max(dot(local, local), 1e-8);
        float pullSquared = max(dot(pulled, pulled), 1e-8);
        float bodyField = uPullShape.x * uPullShape.x / bodySquared;
        float pullField = uPullShape.y * uPullShape.y / pullSquared;
        float field = max(bodyField + pullField, 1e-12);
        float inverseRoot = inversesqrt(field);
        vec2 gradient = bodyField * local / bodySquared + pullField * pulled / pullSquared;
        float slope = length(gradient) * inverseRoot / field;
        float fieldScale = max(length(uPullShape.xy), 0.001);
        distance = radius * (inverseRoot - 1.0) / max(slope, 0.2 / fieldScale) *
                   min(interactionStretch, 1.0 / interactionStretch);
    } else if (uPinned < 0.5 && trailActivation > 0.0005) {
        float slimeDrop = 100000.0;
        int sampleCount = GLASSCOPE_BEZIER_SAMPLES;
        for (int index = 0; index < GLASSCOPE_BEZIER_SAMPLES; ++index) {
            float amount = float(index) / float(sampleCount - 1);
            vec2 materialPoint = cubicBezierPoint(nodes, amount);
            float localStretch = length(cubicBezierDerivative(nodes, amount)) /
                                 max(radius * 1.45, 1.0);
            float materialRadius = radius * clamp(
                0.22 + 0.76 / (1.0 + localStretch * 0.98),
                0.22, 0.96);
            float sampleDrop = circleDistance(point, materialPoint,
                                              materialRadius);
            slimeDrop = smoothMinimum(slimeDrop, sampleDrop,
                                      radius * 0.065);
        }
        distance = mix(restDrop, slimeDrop, trailActivation);
    }

    float surfaceEnergy = clamp(speed + trailActivation * 0.28 +
                                abs(uWobble) * motionStrength * 0.34,
                                0.0, 1.35);
    float axisAngle = atan(across, along);
    float flowRipple = sin(axisAngle * 3.0 + along / radius * 1.7 -
                           uTime * 5.2) * 0.008 +
                       sin(axisAngle * 5.0 + uTime * 3.8) * 0.004;
    distance -= radius * flowRipple * surfaceEnergy;

    float stopMode = cos(axisAngle * 2.0) * 0.033 +
                     sin(axisAngle * 3.0 + 0.65) * 0.012;
    distance -= radius * signedBounce * motionStrength * stopMode;
    return distance;
}

vec2 safeUv(vec2 uv) {
    vec2 texel = 1.5 / uResolution;
    return clamp(uv, texel, uTextureMax - texel);
}

void main() {
    vec2 pixelPoint = (vUv - uCenter) * uResolution;
    float motionStrength;
    float rawSpeed;
    float signedBounce;
    float radius;
    float maximumTrailLength;
    vec2 nodes[4];
    prepareLens(motionStrength, rawSpeed, signedBounce, radius, nodes,
                maximumTrailLength);

    float conservativeOuterRadius = maximumTrailLength + radius * 1.40 + 2.0;
    if (length(pixelPoint) > conservativeOuterRadius) {
        outColor = vec4(0.0);
        return;
    }

    float distance = lensDistance(pixelPoint, motionStrength, rawSpeed,
                                  signedBounce, radius, nodes,
                                  maximumTrailLength);

    vec2 normal = normalize(vec2(dFdx(distance), dFdy(distance)) +
                            vec2(0.000001));

    float antialias = 1.75;
    float mask = 1.0 - smoothstep(-antialias, antialias, distance);
    float edgeWidth = max(uEdgeWidthPx, 4.0);
    float stretch = 1.0 - exp(-maximumTrailLength / max(radius, 1.0) * 2.4);
    vec2 flowAxis = -nodes[1] / max(length(nodes[1]), 0.5);
    float leading = dot(normal, flowAxis);
    float tailDepth = clamp(-dot(pixelPoint, flowAxis) / max(radius, 1.0),
                            0.0, 1.5) / 1.5;
    float compression = signedBounce * motionStrength;
    float thickness = clamp(1.0 + stretch * (leading * 0.20 - tailDepth * 0.22) +
                             compression * 0.15, 0.62, 1.28);
    float opticalWidth = edgeWidth * thickness;
    float rimWide = 1.0 - smoothstep(0.5, edgeWidth, abs(distance));
    float rimCore = 1.0 - smoothstep(0.25, max(2.0, edgeWidth * 0.20),
                                      abs(distance));
    float innerShell = smoothstep(-edgeWidth * 1.55,
                                  -edgeWidth * 0.16, distance) *
                       (1.0 - smoothstep(-edgeWidth * 0.04,
                                         edgeWidth * 0.28, distance));

    float revealRadius = uRadiusPx * mix(0.82, 1.0, uReveal);
    float radial = clamp(length(pixelPoint) / max(revealRadius, 1.0),
                         0.0, 1.0);
    float dome = 1.0 - radial * radial;
    float domeScale = 1.0 - clamp(uBulge, 0.0, 0.28) * dome;
    vec2 magnifiedUv = uCenter +
                       (vUv - uCenter) / max(uZoom, 1.0) * domeScale;

    float centerSafety = smoothstep(0.035, 0.16, radial);
    float edgeRefraction = (rimWide * 9.0 + rimCore * 8.0 +
                            innerShell * 2.8) *
                           uStrength * uEdgeStrength * thickness;
    vec2 refraction = normal / uResolution * edgeRefraction * centerSafety;

    vec2 tangent = vec2(-normal.y, normal.x);
    float wave = sin(dot(pixelPoint, tangent) * 0.055 + uTime * 4.5);
    refraction += tangent / uResolution * wave * abs(uWobble) * 2.8;

    vec2 sampleUv = safeUv(magnifiedUv + refraction);
    vec3 base = texture(uTexture, sampleUv).rgb;
    float sourceLuminance = perceptualLuminance(base);

    float spectralWidth = (rimWide * 3.7 + rimCore * 2.6) * thickness;
    vec2 spectralOffset = normal / uResolution * spectralWidth *
                          uDispersion * uEdgeStrength * centerSafety;
    vec3 refracted = base;
    if (uDispersion > 0.001 && spectralWidth > 0.001) {
        refracted.r = texture(uTexture, safeUv(sampleUv + spectralOffset)).r;
        refracted.b = texture(uTexture, safeUv(sampleUv - spectralOffset)).b;
    }

    vec3 mint = vec3(0.545, 0.890, 0.831);
    vec3 lilac = vec3(0.804, 0.729, 1.000);
    float tintPhase = 0.5 + 0.5 * sin(pixelPoint.x * 0.016 +
                                      pixelPoint.y * 0.011 + uTime * 0.8);
    vec3 tint = mix(mint, lilac, tintPhase);
    tint = preserveLuminance(tint, sourceLuminance);
    float glassTint = rimWide * 0.105 + innerShell * 0.055;
    refracted = mix(refracted, tint,
                    glassTint * uStrength * uEdgeStrength);
    refracted = preserveLuminance(refracted, sourceLuminance);

    vec2 lightDirection = normalize(vec2(-0.72, -0.55));
    float lightFacing = max(dot(-normal, lightDirection), 0.0);
    vec3 highlightColor = vec3(0.91, 1.0, 0.98);
    float colorStrength = clamp(uColorStrength, 0.0, 1.0);
    if (colorStrength > 0.0) {
        float colorWidth = max(uColorWidthPx, 1.0);
        float colorInner = smoothstep(-colorWidth * 1.55,
                                      -colorWidth * 0.16, distance) *
                           (1.0 - smoothstep(-colorWidth * 0.04,
                                             colorWidth * 0.28, distance));
        float colorGrazing = pow(clamp(1.0 + distance / (colorWidth * 1.55),
                                       0.0, 1.0), 1.5);
        float backFacing = max(dot(normal, lightDirection), 0.0);
        vec3 absorption = 1.0 - clamp(uTransmissionColor.rgb, 0.0, 1.0);
        refracted *= exp(-absorption * clamp(uTransmissionColor.a, 0.0, 1.0) *
                         colorStrength * (0.08 + colorInner * 0.65));
        float refractionAmount = colorInner * (0.55 + 0.45 * backFacing) *
                                 clamp(uRefractionColor.a, 0.0, 1.0) *
                                 uStrength * uEdgeStrength * 0.85;
        refracted = mix(refracted, clamp(uRefractionColor.rgb, 0.0, 1.0),
                        colorStrength * clamp(refractionAmount, 0.0, 0.45));
        float reflectionAmount = colorGrazing * (0.10 + 0.38 * lightFacing * lightFacing) *
                                 clamp(uReflectionColor.a, 0.0, 1.0) * uEdgeStrength;
        refracted = mix(refracted, clamp(uReflectionColor.rgb, 0.0, 1.0),
                        colorStrength * clamp(reflectionAmount, 0.0, 0.45));
        highlightColor = mix(highlightColor, clamp(uHighlightColor.rgb, 0.0, 1.0),
                             colorStrength * clamp(uHighlightColor.a, 0.0, 1.0));
    }
    float surfaceActivity = clamp(stretch * 0.75 +
                                   abs(compression) * 0.65, 0.0, 1.0);
    float filamentWidth = max(1.0 * uScale, opticalWidth * 0.052);
    float filamentDepth = opticalWidth * (0.10 + surfaceActivity * 0.055);
    float filament = 1.0 - smoothstep(filamentWidth * 0.25, filamentWidth,
                                      abs(distance + filamentDepth));
    float secondaryDepth = filamentDepth + opticalWidth *
                           (0.12 + surfaceActivity * 0.15);
    float secondary = 1.0 - smoothstep(filamentWidth * 0.35,
                                       filamentWidth * 1.35,
                                       abs(distance + secondaryDepth));
    vec2 secondaryLight = normalize(lightDirection +
                                    vec2(-lightDirection.y, lightDirection.x) *
                                    (0.16 + surfaceActivity * 0.20));
    float secondaryFacing = max(dot(-normal, secondaryLight), 0.0);
    float highlight = pow(lightFacing, 4.0) *
                      (rimCore * 0.52 + innerShell * 0.24) +
                      pow(lightFacing, 12.0) * filament * 0.68 +
                      pow(secondaryFacing, 20.0) * secondary *
                      (0.16 + surfaceActivity * 0.30);
    float innerShade = pow(max(dot(normal, lightDirection), 0.0), 2.0) *
                       innerShell;
    refracted *= 1.0 - innerShade * 0.10 * uEdgeStrength;
    refracted += highlightColor * highlight *
                 0.42 * uEdgeStrength;

    float gatherDepth = opticalWidth * (0.60 + compression * 0.08);
    float gatherWidth = max(1.2 * uScale, opticalWidth * 0.10);
    float gatheredLight = 1.0 - smoothstep(gatherWidth * 0.2, gatherWidth,
                                          abs(distance + gatherDepth));
    float gatheredShade = 1.0 - smoothstep(gatherWidth * 0.4,
                                          gatherWidth * 1.8,
                                          abs(distance + gatherDepth +
                                              gatherWidth * 1.8));
    float backLight = pow(max(dot(normal, lightDirection), 0.0), 6.0);
    float gatherAmount = backLight * (0.032 + surfaceActivity * 0.05) *
                         thickness * uStrength * uEdgeStrength;
    refracted *= 1.0 - gatheredShade * gatherAmount * 0.65;
    refracted += highlightColor * gatheredLight * gatherAmount;

    float probeAmount = clamp(uColorProbeAmount, 0.0, 1.0);
    if (probeAmount > 0.001) {
        float probeScale = max(uScale, 0.25);
        vec2 probePoint = pixelPoint / probeScale;
        float horizontal =
            (1.0 - smoothstep(0.55, 1.15, abs(probePoint.y))) *
            smoothstep(3.8, 5.0, abs(probePoint.x)) *
            (1.0 - smoothstep(12.0, 13.2, abs(probePoint.x)));
        float vertical =
            (1.0 - smoothstep(0.55, 1.15, abs(probePoint.x))) *
            smoothstep(3.8, 5.0, abs(probePoint.y)) *
            (1.0 - smoothstep(12.0, 13.2, abs(probePoint.y)));
        float crosshair = max(horizontal, vertical);
        float horizontalOutline =
            (1.0 - smoothstep(1.15, 1.85, abs(probePoint.y))) *
            smoothstep(3.1, 4.1, abs(probePoint.x)) *
            (1.0 - smoothstep(12.8, 14.0, abs(probePoint.x)));
        float verticalOutline =
            (1.0 - smoothstep(1.15, 1.85, abs(probePoint.x))) *
            smoothstep(3.1, 4.1, abs(probePoint.y)) *
            (1.0 - smoothstep(12.8, 14.0, abs(probePoint.y)));
        float crosshairOutline = max(horizontalOutline, verticalOutline);

        vec3 liveProbeColor = texture(uTexture, safeUv(uCenter)).rgb;
        vec3 probeColor = mix(liveProbeColor, uColorProbeColor,
                              clamp(uColorProbeCaptured, 0.0, 1.0));
        float probeLuminance = perceptualLuminance(probeColor);
        vec3 ink = probeLuminance < 0.48 ? vec3(1.0) : vec3(0.04);
        vec3 shadowInk = probeLuminance < 0.48 ? vec3(0.04) : vec3(1.0);

        refracted = mix(refracted, shadowInk,
                        crosshairOutline * probeAmount * 0.72);
        refracted = mix(refracted, ink, crosshair * probeAmount);

        float logicalRadius = radius / probeScale;
        vec2 swatchPoint = probePoint - vec2(0.0, logicalRadius * 0.60);
        float swatchOuter = 1.0 - smoothstep(
            -0.4, 0.8,
            roundedBoxDistance(swatchPoint, vec2(18.0, 9.0), 4.0));
        float swatchInner = 1.0 - smoothstep(
            -0.4, 0.8,
            roundedBoxDistance(swatchPoint, vec2(15.0, 6.0), 2.5));
        float swatchBorder = max(swatchOuter - swatchInner, 0.0);
        refracted = mix(refracted, ink,
                        swatchBorder * probeAmount * 0.86);
        refracted = mix(refracted, probeColor,
                        swatchInner * probeAmount);
    }

    float alpha = mask * uReveal;
#ifdef GLASSCOPE_PLUGIN
    vec4 result = vec4(refracted * alpha, alpha);
#else
    vec4 result = vec4(refracted, alpha) * fragColor;
#endif
    outColor = result;
}
