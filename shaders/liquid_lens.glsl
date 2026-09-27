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

    float antialias = 1.75;
    float mask = 1.0 - smoothstep(-antialias, antialias, distance);
    vec3 refracted = lensOptics(pixelPoint, distance, radius, signedBounce,
                                motionStrength, maximumTrailLength, nodes);
    refracted = probeOverlay(refracted, pixelPoint, radius);

    float alpha = mask * uReveal;
#ifdef GLASSCOPE_PLUGIN
    vec4 result = vec4(refracted * alpha, alpha);
#else
    vec4 result = vec4(refracted, alpha) * fragColor;
#endif
    outColor = result;
}
