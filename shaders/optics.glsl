float perceptualLuminance(vec3 color) {
    return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

vec3 preserveLuminance(vec3 color, float targetLuminance) {
    float difference = targetLuminance - perceptualLuminance(color);
    return clamp(color + vec3(difference), 0.0, 1.0);
}

vec2 safeUv(vec2 uv) {
    vec2 texel = 1.5 / uResolution;
    return clamp(uv, texel, uTextureMax - texel);
}

vec3 lensOptics(vec2 pixelPoint, float distance, float radius, float signedBounce,
                float motionStrength, float maximumTrailLength, vec2 nodes[4]) {
    vec2 normal = normalize(vec2(dFdx(distance), dFdy(distance)) +
                            vec2(0.000001));

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

    return refracted;
}
