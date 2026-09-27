#include "glasscope/lens_config.hpp"
#include "glasscope/lens_geometry.hpp"
#include "glasscope/lens_state.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace Glasscope;

void require(bool value, const char* message) {
    if (!value)
        throw std::runtime_error(message);
}

bool near(Vec2 a, Vec2 b) {
    return length(subtract(a, b)) < 0.001;
}

void advance(LensState& state, double from, double to, double fps = 120.0) {
    for (double t = from; t <= to; t += 1.0 / fps)
        state.advance(t);
}

int main() {
    for (const double fps : {30.0, 60.0, 144.0, 240.0}) {
        LensState state;
        state.setVisible(true, {300.0, 300.0}, 0.0);
        advance(state, 0.0, 1.0, fps);
        require(!state.beginPress({300.0, 300.0}, 1.0), "Following lens must pass clicks through");
        state.setPinned(true, {300.0, 300.0}, 1.0);
        state.move({900.0, 900.0}, 1.01);
        require(near(state.snapshot().center, {300.0, 300.0}), "Pinned lens followed pointer");
        require(state.beginPress({350.0, 330.0}, 1.02), "Pinned press rejected");
        require(!state.beginPress({350.0, 330.0}, 1.025), "Duplicate press accepted");
        state.move({351.0, 330.0}, 1.03);
        require(near(state.snapshot().center, {300.0, 300.0}), "Click jitter moved lens");
        double pressPeak = 0.0;
        double previousPressure = 0.0;
        for (double t = 1.03; t <= 1.08; t += 1.0 / fps) {
            state.advance(t);
            const double pressure = state.snapshot().interactionWobble;
            require(pressure <= previousPressure + 0.00001, "Press oscillates instead of compressing");
            require(pressure >= -0.10001, "Press unexpectedly starts a bounce");
            previousPressure = pressure;
            pressPeak = std::max(pressPeak, std::abs(pressure));
        }
        require(pressPeak > 0.06, "Press compression is missing");
        state.move({400.0, 380.0}, 1.09);
        require(near(state.snapshot().center, {300.0, 300.0}), "Pull moved fixed anchor");
        state.advance(1.10);
        require(length(state.snapshot().trailNodes[2]) > 25.0, "Pull did not deform lens");
        LensState elastic = state;
        const Vec2 heldPull = elastic.snapshot().trailNodes[2];
        advance(elastic, 1.11, 1.80, fps);
        require(near(elastic.snapshot().trailNodes[2], heldPull), "Held deformation drifted");
        require(!elastic.needsAnimation(), "Held stationary pull keeps animating");
        elastic.endPress(1.81);
        bool overshot = false;
        for (int i = 1; i < 100; ++i) {
            elastic.advance(1.81 + i / fps);
            const Vec2 pull = elastic.snapshot().trailNodes[2];
            overshot = overshot || pull.x < -0.5;
            require(near(elastic.snapshot().center, {300.0, 300.0}), "Elastic return moved anchor");
        }
        require(overshot, "Release lacks elastic overshoot");
        state.endPress(1.11);
        state.move({900.0, 900.0}, 1.12);
        require(near(state.snapshot().center, {300.0, 300.0}), "Release moved fixed anchor");
        advance(state, 1.12, 1.16, fps);
        require(length(state.snapshot().trailNodes[2]) > 1.0, "Pull release has no elastic feedback");
        advance(state, 1.17, 3.0, fps);
        require(!state.needsAnimation(), "Released lens never settles");
        require(near(state.snapshot().center, {300.0, 300.0}), "Rebound moved lens center");
        state.setVisible(false, {900.0, 900.0}, 3.01);
        require(!state.beginPress({300.0, 300.0}, 3.02), "Hidden lens captures input");
        state.setVisible(true, {900.0, 900.0}, 3.03);
        require(near(state.snapshot().center, {300.0, 300.0}), "Show lost pinned position");
        state.beginPress({300.0, 300.0}, 3.04);
        state.setVisible(false, {0.0, 0.0}, 3.05);
        require(!state.pressed(), "Hide did not cancel press");
        state.setVisible(true, {900.0, 900.0}, 3.06);
        state.setPinned(false, {900.0, 900.0}, 3.07);
        state.move({910.0, 900.0}, 3.08);
        require(near(state.snapshot().center, {910.0, 900.0}), "Unpin did not restore following");
        require(length(state.snapshot().trailNodes[2]) < 20.0, "Unpin created long connecting tail");
        state.setPinned(true, {400.0, 300.0}, 3.09);
        state.beginPress({500.0, 300.0}, 3.10);
        state.move({10.0, 300.0}, 3.11);
        require(near(state.snapshot().center, {400.0, 300.0}), "Long pull moved anchor");
        require(length(state.snapshot().trailNodes[2]) < 235.0, "Long pull is unbounded");
        state.endPress(3.12);
        state.move({700.0, 500.0}, 3.13);
        require(near(state.snapshot().center, {400.0, 300.0}), "Released anchor followed pointer");
        state.reset();
        require(!state.snapshot().pinned && !state.pressed(), "Reset retained interaction state");
    }
    LensStyle style;
    LensState idle;
    idle.setVisible(true, {300.0, 300.0}, 0.0);
    idle.setPinned(true, {300.0, 300.0}, 0.0);
    advance(idle, 0.0, 1.0);
    idle.beginPress({300.0, 300.0}, 10.0);
    idle.advance(10.025);
    require(idle.snapshot().interactionWobble < -0.04 && idle.snapshot().interactionWobble > -0.10,
            "Idle press did not compress gently");
    idle.endPress(10.04);
    idle.advance(10.065);
    require(std::abs(idle.snapshot().interactionWobble) > 0.3, "Click release did not trigger its single duang");
    idle.beginPress({300.0, 300.0}, 11.0);
    idle.move({600.0, 300.0}, 11.03);
    idle.advance(11.05);
    idle.endPress(15.0);
    idle.advance(15.025);
    require(length(idle.snapshot().trailNodes[2]) > 20.0, "Long hold snapped back without animation");
    require(std::abs(idle.snapshot().interactionWobble) < 0.10, "Pull release added a second scalar bounce");
    style.radius = 130.0F;
    LensSnapshot snapshot;
    snapshot.center = {300.0, 300.0};
    snapshot.requestedVisible = true;
    snapshot.reveal = 1.0;
    for (double scale : {1.0, 1.25, 1.5, 2.0}) {
        require(lensContains({300.0, 300.0}, snapshot, style, scale), "Center not clickable");
        require(lensContains({420.0, 300.0}, snapshot, style, scale), "Interior not clickable");
        require(!lensContains({425.0, 425.0}, snapshot, style, scale), "Transparent corner captures click");
        require(!lensContains({500.0, 300.0}, snapshot, style, scale), "Outside captures click");
        snapshot.trailNodes = {Vec2{-60.0, 0.0}, Vec2{-100.0, 0.0}, Vec2{-120.0, 0.0}};
        require(lensContains({160.0, 300.0}, snapshot, style, scale), "Visible tail not clickable");
        require(!lensContains({160.0, 430.0}, snapshot, style, scale), "Tail bounds capture transparent corner");
        snapshot.pinned = true;
        snapshot.trailNodes = {Vec2{30.0, 0.0}, Vec2{60.0, 0.0}, Vec2{90.0, 0.0}};
        require(lensContains({475.0, 300.0}, snapshot, style, scale), "Pulled tip not clickable");
        require(!lensContains({165.0, 300.0}, snapshot, style, scale), "Rear edge recoils against the pull");
        require(lensContains({220.0, 300.0}, snapshot, style, scale), "Anchored body vanished");
        require(!lensContains({430.0, 430.0}, snapshot, style, scale), "Pull corner captures input");
        require(!lensContains({300.0, 440.0}, snapshot, style, scale), "Body expands beyond the resting side");
        snapshot.pinned = false;
        snapshot.trailNodes = {};
    }
    auto area = [&](const LensSnapshot& shape) {
        int samples = 0;
        for (int y = -600; y <= 600; y += 2)
            for (int x = -600; x <= 600; x += 2)
                samples += lensContains({shape.center.x + x, shape.center.y + y}, shape, style, 1.0) ? 1 : 0;
        return static_cast<double>(samples) * 4.0;
    };
    snapshot.pinned = true;
    const double restArea = area(snapshot);
    double worstAreaChange = 0.0;
    for (double angle : {0.0, 0.7}) {
        snapshot.pullAxis = {std::cos(angle), std::sin(angle)};
        for (double pull : {-25.0, 30.0, 70.0, 110.0, 230.0}) {
            const Vec2 vector = multiply(snapshot.pullAxis, pull);
            snapshot.trailNodes = {multiply(vector, 0.33), multiply(vector, 0.66), vector};
            const double relativeChange = std::abs(area(snapshot) / restArea - 1.0);
            worstAreaChange = std::max(worstAreaChange, relativeChange);
            require(relativeChange < 0.025, "Whole-body pull changes projected area too much");
        }
    }
    std::cout << "Maximum sampled hit-area change: " << worstAreaChange * 100.0 << "%\n";
    for (double angle : {0.0, 0.7, 1.8, 2.7}) {
        snapshot.pullAxis = {std::cos(angle), std::sin(angle)};
        for (double pull : {10.0, 50.0, 110.0, 230.0, 500.0}) {
            const Vec2 vector = multiply(snapshot.pullAxis, pull);
            snapshot.trailNodes = {multiply(vector, 0.33), multiply(vector, 0.66), vector};
            const Vec2 outside = {snapshot.center.x - snapshot.pullAxis.x * 133.0,
                                  snapshot.center.y - snapshot.pullAxis.y * 133.0};
            const Vec2 inside = {snapshot.center.x - snapshot.pullAxis.x * 80.0,
                                 snapshot.center.y - snapshot.pullAxis.y * 80.0};
            require(!lensContains(outside, snapshot, style, 1.0), "Strong pull pushes the rear edge backwards");
            require(lensContains(inside, snapshot, style, 1.0), "Strong pull loses the anchored body");
        }
    }
    // Grip depth sets the allocation once, with no larger-than-half pulled drop.
    double previousShare = 0.0;
    for (double grip : {129.0, 100.0, 65.0, 25.0, 0.0}) {
        LensState state;
        state.setVisible(true, {0.0, 0.0}, 0.0);
        state.advance(1.0);
        state.setPinned(true, {0.0, 0.0}, 1.0);
        state.beginPress({grip, 0.0}, 1.01);
        const double share = state.snapshot().pullShare;
        require(share > previousShare && share <= 0.5, "Grip depth does not set a bounded increasing share");
        for (int step = 0; step < 40; ++step) {
            state.move({grip + step * 20.0, step * 4.0}, 1.02 + step / 120.0);
            state.advance(1.02 + step / 120.0);
            require(std::abs(state.snapshot().pullShare - share) < 0.000001,
                    "Share changes as the pointer leaves the grip");
        }
        previousShare = share;
    }
    require(std::abs(previousShare - 0.5) < 0.000001, "Center grip should allocate half");
    for (double share : {0.06, 0.15, 0.28, 0.5}) {
        double previousBody = 1.001;
        for (int step = 1; step <= 52; ++step) {
            const PinnedShape shape = pinnedShape(step * 0.05, share);
            const double actualShare = shape.pullRadius * shape.pullRadius /
                                       (shape.pullRadius * shape.pullRadius + shape.bodyRadius * shape.bodyRadius);
            require(std::abs(actualShare - share) < 0.000001, "Area normalization changes the allocated ratio");
            require(shape.bodyRadius <= previousBody + 0.002, "Anchored body recoils as the pull grows");
            previousBody = shape.bodyRadius;
            for (int sample = 0; sample <= 100; ++sample)
                require(pinnedDistance({shape.separation * sample / 100.0, 0.0}, shape) < -0.015,
                        "Pulled bubble disconnects from the body");
        }
    }
    // The field must emerge continuously from the resting circle at every grip
    // depth, and preserve visible area even when the small lobe nearly pinches.
    double worstFieldAreaChange = 0.0;
    for (double share : {0.06, 0.15, 0.28, 0.5}) {
        const PinnedShape onset = pinnedShape(0.0001, share);
        for (int i = 0; i < 64; ++i) {
            const double angle = i * 2.0 * std::acos(-1.0) / 64.0;
            require(std::abs(pinnedDistance({std::cos(angle), std::sin(angle)}, onset)) < 0.002,
                    "Beginning a drag pops away from the resting circle");
        }
        for (double strain : {0.3, 1.2, 2.6}) {
            const PinnedShape shape = pinnedShape(strain, share);
            int inside = 0;
            for (int y = 0; y < 120; ++y) {
                for (int x = -130; x < 330; ++x) {
                    const Vec2 point = {(x + 0.5) * 0.01, (y + 0.5) * 0.01};
                    const double distance = pinnedDistance(point, shape);
                    require(std::isfinite(distance), "Metaball distance is not finite");
                    inside += distance <= 0.0 ? 1 : 0;
                }
            }
            const double error = std::abs(inside * 0.0002 / std::acos(-1.0) - 1.0);
            worstFieldAreaChange = std::max(worstFieldAreaChange, error);
            require(error < 0.006, "Metaball silhouette loses projected area");
            require(std::isfinite(pinnedDistance({0.0, 0.0}, shape)) &&
                        std::isfinite(pinnedDistance({shape.separation, 0.0}, shape)),
                    "Metaball source has a distance singularity");
        }
    }
    std::cout << "Maximum sampled field-area change: " << worstFieldAreaChange * 100.0 << "%\n";
    // More pull should increase both reach and the relative release overshoot.
    double previousReach = 0.0, previousOvershoot = 0.0;
    for (double distance : {30.0, 150.0, 500.0, 1500.0}) {
        LensState release;
        release.setVisible(true, {0.0, 0.0}, 0.0);
        release.advance(1.0);
        release.setPinned(true, {0.0, 0.0}, 1.0);
        release.beginPress({0.0, 0.0}, 1.01);
        release.move({distance, 0.0}, 1.02);
        const double reach = release.snapshot().trailNodes[2].x;
        require(reach > previousReach && reach < 234.0, "Extended pull range is not bounded and monotonic");
        release.endPress(1.03);
        double overshoot = 0.0;
        for (int i = 1; i <= 480; ++i) {
            release.advance(1.03 + i / 240.0);
            overshoot = std::max(overshoot, -release.snapshot().trailNodes[2].x / reach);
        }
        require(overshoot > previousOvershoot, "Release strength does not increase relative oscillation");
        require(!release.needsAnimation(), "Strong release never settles");
        previousReach = reach;
        previousOvershoot = overshoot;
    }
    require(previousReach > 200.0, "Maximum reach did not increase beyond the old 110.5px cap");
    // Release keeps the current contour; only velocity changes. Integrating the
    // same spring interval in different frame partitions gives the same result.
    double referenceClick = 0.0;
    for (int fps : {30, 60, 144, 240}) {
        LensState click;
        click.setVisible(true, {0.0, 0.0}, 0.0);
        click.advance(1.0);
        click.setPinned(true, {0.0, 0.0}, 1.0);
        click.beginPress({0.0, 0.0}, 1.0);
        click.advance(1.05);
        click.advance(1.10);
        const auto pressed = click.snapshot();
        click.endPress(1.10);
        require(std::abs(click.snapshot().interactionWobble - pressed.interactionWobble) < 1e-8,
                "Click release jumps its pressure state");
        const int frames = fps / 6;
        const double step = 0.2 / frames;
        double peak = 0.0;
        for (int i = 1; i <= frames; ++i) {
            click.advance(1.10 + i * step);
            peak = std::max(peak, std::abs(click.snapshot().interactionWobble));
        }
        require(peak > 0.4, "Click spring is too weak to read");
        const double finalClick = click.snapshot().interactionWobble;
        if (fps == 30)
            referenceClick = finalClick;
        require(std::abs(finalClick - referenceClick) < 1e-7, "Click spring changes with frame partition");
        const auto beforeRegrab = click.snapshot();
        click.beginPress({0.0, 0.0}, 1.30);
        require(std::abs(click.snapshot().interactionWobble - beforeRegrab.interactionWobble) < 1e-7,
                "Re-grabbing a bounce jumps the surface");
        click.endPress(1.31);
        advance(click, 1.32, 4.0, fps);
        require(!click.needsAnimation(), "Repeated click spring never settles");
    }
    LensState flick;
    flick.setVisible(true, {0.0, 0.0}, 0.0);
    flick.advance(1.0);
    flick.setPinned(true, {0.0, 0.0}, 1.0);
    flick.beginPress({0.0, 0.0}, 1.0);
    for (int i = 1; i <= 12; ++i) {
        flick.move({i * 15.0, 0.0}, 1.0 + i / 120.0);
        flick.advance(1.0 + i / 120.0);
    }
    LensState paused = flick;
    const auto held = flick.snapshot();
    flick.endPress(1.1);
    require(near(flick.snapshot().trailNodes[2], held.trailNodes[2]), "Flick release jumps its position");
    flick.advance(1.12);
    advance(paused, 1.11, 1.4);
    paused.endPress(1.4);
    paused.advance(1.42);
    require(flick.snapshot().trailNodes[2].x > paused.snapshot().trailNodes[2].x + 1.0,
            "Release does not distinguish recent and stale drag velocity");
    double compressionPeak = 0.0;
    for (int i = 1; i <= 480; ++i) {
        flick.advance(1.12 + i / 240.0);
        compressionPeak = std::max(compressionPeak, -flick.snapshot().interactionWobble);
        require(length(flick.snapshot().trailNodes[2]) < 260.0, "Inherited release velocity explodes");
    }
    require(compressionPeak > 0.25 && !flick.needsAnimation(), "Flick squash is absent or never settles");

    snapshot.trailNodes = {};
    snapshot.pullAxis = {1.0, 0.0};
    snapshot.interactionWobble = 0.7;
    style.interactionBounce = 0.0F;
    require(!lensContains({445.0, 300.0}, snapshot, style, 1.0), "Zero interaction gain still stretches the lens");
    style.interactionBounce = 1.0F;
    require(lensContains({445.0, 300.0}, snapshot, style, 1.0), "Default interaction gain has no visible bounce");
    require(!lensContains({462.0, 300.0}, snapshot, style, 1.0), "Default interaction gain is unbounded");
    style.interactionBounce = 2.5F;
    require(lensContains({462.0, 300.0}, snapshot, style, 1.0), "Higher interaction gain does not amplify bounce");
    require(!lensContains({300.0, 420.0}, snapshot, style, 1.0), "Stretch does not compress the transverse axis");
    snapshot.interactionWobble = 0.0;
    style.interactionBounce = 1.0F;
    // Vary complete click gestures, never their individual rendered frames.
    LensState varied;
    varied.setVisible(true, {0.0, 0.0}, 0.0);
    varied.advance(1.0);
    varied.setPinned(true, {0.0, 0.0}, 1.0);
    Vec2 previousAxis = {1.0, 0.0};
    int differentAxes = 0;
    double smallestPeak = 10.0, largestPeak = 0.0;
    for (int click = 0; click < 12; ++click) {
        const double start = 1.0 + click * 2.0;
        varied.beginPress({0.0, 0.0}, start);
        const Vec2 axis = varied.snapshot().pullAxis;
        require(std::abs(length(axis) - 1.0) < 1e-10, "Random click direction is not normalized");
        differentAxes += std::abs(axis.x * previousAxis.x + axis.y * previousAxis.y) < 0.9 ? 1 : 0;
        previousAxis = axis;
        varied.advance(start + 0.05);
        varied.advance(start + 0.10);
        varied.endPress(start + 0.10);
        double peak = 0.0;
        for (int frame = 1; frame <= 420; ++frame) {
            varied.advance(start + 0.10 + frame / 240.0);
            peak = std::max(peak, varied.snapshot().interactionWobble);
            require(near(varied.snapshot().pullAxis, axis), "Click axis jitters between frames");
        }
        smallestPeak = std::min(smallestPeak, peak);
        largestPeak = std::max(largestPeak, peak);
        require(!varied.needsAnimation(), "Varied click does not settle");
    }
    require(differentAxes >= 8, "Settled clicks still use nearly one direction");
    require(smallestPeak > 0.4 && largestPeak < 0.8 && largestPeak - smallestPeak > 0.025,
            "Click amplitude variation is missing or excessive");

    std::array<double, 4> referenceWave = {};
    for (double fps : {30.0, 60.0, 144.0, 240.0}) {
        LensState soft;
        soft.setVisible(true, {0.0, 0.0}, 0.0);
        soft.advance(1.0);
        soft.setPinned(true, {0.0, 0.0}, 1.0);
        soft.beginPress({0.0, 0.0}, 1.0);
        soft.move({600.0, 0.0}, 1.02);
        advance(soft, 1.02, 1.50, fps);
        soft.endPress(1.50);
        double time = 1.50;
        for (int sample = 0; sample < 4; ++sample) {
            const double target = 1.70 + sample * 0.20;
            while (time < target - 1e-10) {
                time = std::min(time + 1.0 / fps, target);
                soft.advance(time);
            }
            const double wave = soft.snapshot().wobble;
            if (fps == 30.0)
                referenceWave[sample] = wave;
            require(std::abs(wave - referenceWave[sample]) < 1e-7, "Merge-to-shiver handoff depends on frame rate");
        }
        advance(soft, 2.31, 4.0, fps);
        require(!soft.needsAnimation(), "Surface shiver never settles");
    }
    require(std::abs(referenceWave[0]) > 0.05 && std::abs(referenceWave[2]) > 0.004,
            "Drag return lacks a visible decaying shiver");
    require(std::abs(referenceWave[3]) < std::abs(referenceWave[0]), "Shiver fails to decay");

    snapshot.requestedVisible = false;
    require(!lensContains({300.0, 300.0}, snapshot, style, 1.0), "Closing lens captures input");
    for (double radius : {80.0, 130.0, 600.0}) {
        const double extent = lensExtent({.radius = radius, .motionStrength = 1.5, .maximumTrailLength = radius});
        require(extent > radius * 2.35, "Render bounds clip maximally pulled tip");
    }
    std::cout << "Interaction checks passed at 30/60/144/240 Hz and 1/1.25/1.5/2 scale\n";
}
