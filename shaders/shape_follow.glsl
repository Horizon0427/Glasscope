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

float followingLensDistance(vec2 point, vec2 nodes[4], float radius, float restDrop, float trailActivation) {
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
        return mix(restDrop, slimeDrop, trailActivation);
}
