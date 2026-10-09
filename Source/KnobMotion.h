#pragma once

// The value the knob shows, one display frame at a time (docs/ui-spec.md "Knob" motion, D-25). Jumps glide
// to the new angle; everything else follows 1:1. Only the picture eases: the parameter never does.
class KnobMotion
{
public:
    // How a change of value reaches the screen.
    enum class Change
    {
        judgeBySize,  // a host change: glides only if it moved more than UiSpec::Knob::glideJumpPercent this frame
        follow,       // the knob's own drags and swipes: always 1:1, so the knob never lags the hand
        glide         // the knob's own notches and resets: always glides
    };

    explicit KnobMotion (float initialPercent);

    // Call once per display frame with the parameter's value. Returns true when the shown value moved.
    bool advance (float targetPercent, double nowSeconds, Change change);

    float getShownPercent() const { return shownPercent; }

private:
    void moveAlongGlide (double nowSeconds);

    float shownPercent;
    float targetPercent;
    float glideStartPercent = 0.0f;
    double glideStartSeconds = 0.0;
    bool gliding = false;
};
