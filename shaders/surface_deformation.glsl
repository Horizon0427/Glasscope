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
        distance = pinnedLensDistance(along, across, radius);
    } else if (uPinned < 0.5 && trailActivation > 0.0005) {
        distance = followingLensDistance(point, nodes, radius, restDrop, trailActivation);
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

