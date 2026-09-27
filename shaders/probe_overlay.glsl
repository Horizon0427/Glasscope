float roundedBoxDistance(vec2 point, vec2 halfSize, float radius) {
    vec2 offset = abs(point) - halfSize + vec2(radius);
    return min(max(offset.x, offset.y), 0.0) +
           length(max(offset, vec2(0.0))) - radius;
}

vec3 probeOverlay(vec3 refracted, vec2 pixelPoint, float radius) {
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

    return refracted;
}
