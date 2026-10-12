#pragma once

// How far the skyline has faded to its bypassed look (docs/ui-spec.md "Skyline" bypassed, PRD §7): over
// 200 ms with a cubic ease-in-out. Toggling mid-fade turns back from where it is, so the picture never jumps.
class SkylineBypassFade
{
public:
    explicit SkylineBypassFade (bool initiallyBypassed);

    // Call on every repaint tick. Returns the bypassed look's share of the picture, 0–1, eased.
    float advance (bool bypassed, double nowSeconds);

private:
    float progress;            // linear: 0 is the normal look, 1 the bypassed one
    float targetProgress;
    float fadeStartProgress;
    double fadeStartSeconds = 0.0;
};
