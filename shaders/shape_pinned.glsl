float pinnedLensDistance(float along, float across, float radius) {
        float interactionStretch = max(uInteractionStretch, 0.001);
        vec2 local = vec2(along / interactionStretch, across * interactionStretch) / radius;
        // Keep this distance estimate in sync with CPU pinnedDistance().
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
        return radius * (inverseRoot - 1.0) / max(slope, 0.2 / fieldScale) *
                   min(interactionStretch, 1.0 / interactionStretch);
}
