#pragma once

#include <juce_graphics/juce_graphics.h>

// Every UI position, size and interaction constant BigCity uses. Each comment names its section in
// docs/ui-spec.md (or, where stated, docs/PRD.md §7). Change a value here only after changing the spec.
// All positions are in points, relative to the window's top-left unless stated otherwise.
namespace UiSpec
{
    // "Window and layout"
    constexpr int windowWidth = 768;
    constexpr int windowHeight = 416;

    // "Draw order" layer 1: the panel fills the window
    inline const juce::Rectangle<float> panelBounds { 0.0f, 0.0f, (float) windowWidth, (float) windowHeight };

    // "Performance": the repaint timer for everything but the knob, which follows the display (D-25)
    constexpr int repaintRateHz = 30;

    namespace Knob
    {
        // "Draw order" layers 4 and 5: knob_base and knob_rotor share these bounds, the knob's origin minus 8
        inline const juce::Rectangle<int> imageBounds { 24, 136, 192, 192 };

        // "Draw order" layer 5: the knob's centre, image-local. The rotor turns about it and the cap is centred on it.
        inline const juce::Point<float> centre { 96.0f, 96.0f };

        // "Knob": the cap (the skirt). Its circle is the hit area and the hover and press overlay (layer 6).
        constexpr float capRadius = 56.0f;

        // "Knob" pointer, from Figma I5:101;5:51: a 4 × 20 rounded rect at 12 o'clock, its inner end 30
        // above the centre. Figma's inset 0 1 0 0 is a 1 pt highlight along its outer end.
        constexpr float pointerWidth = 4.0f;
        constexpr float pointerLength = 20.0f;
        constexpr float pointerCornerRadius = 2.0f;
        constexpr float pointerInnerEndFromCentre = 30.0f;
        constexpr float pointerHighlightThickness = 1.0f;

        // "Knob": angle = −135 + 2.7 × value, in degrees clockwise from 12 o'clock
        constexpr float angleAtZeroDegrees = -135.0f;
        constexpr float degreesPerPercent = 2.7f;

        // "Knob" input and PRD §7: drag about 200 px for the full range; Shift drags in 0.1 steps
        constexpr float dragPixelsForFullRange = 200.0f;
        constexpr float fineDragPercentPerPixel = 0.1f;

        // PRD §7 "Scroll wheel / trackpad", agreed Day 7: 1.0 per notch, 0.1 with Shift
        constexpr float wheelNotchPercent = 1.0f;
        constexpr float fineWheelNotchPercent = 0.1f;

        // JUCE scales macOS trackpad deltas by 0.5 / 256 per pixel; undoing that lets a swipe move the
        // value as far as a drag of the same distance (agreed Day 7: proportional to the swipe).
        constexpr float trackpadPixelsPerWheelUnit = 512.0f;

        // "Knob" input, agreed Day 7: a light trackpad brush must not move the knob, so a scroll burst's
        // movement is ignored until it adds up to this many pixels. Chosen Day 7; tune by feel.
        constexpr float trackpadDeadZonePixels = 8.0f;

        // Wheel events carry no start or end, so a burst ends after this much quiet. Not in the spec:
        // chosen Day 7 so one flick records as one automation gesture.
        constexpr int scrollGestureEndMs = 300;

        // "Knob" motion (D-25): a jump glides to the new angle over 80 ms with a cubic ease-out. A host
        // change counts as a jump when it moves more than 1.0 in one display frame.
        constexpr double glideMs = 80.0;
        constexpr float glideJumpPercent = 1.0f;
    }

    namespace Skyline
    {
        // "Draw order" layers 2 and 3: the screen, which the skyline and screen_overlay fill
        inline const juce::Rectangle<int> screenBounds { 240, 104, 336, 280 };

        // "Draw order" layer 3, and docs/practical-ui.md D7: the bezel's inner corners, which the opaque
        // skyline images are clipped to
        constexpr float screenCornerRadius = 14.0f;

        // "Skyline" grid: 112 × 95 logical pixels at 3 pt each; a window is one logical pixel
        constexpr int gridWidth = 112;
        constexpr int gridHeight = 95;
        constexpr float pointsPerGridPixel = 3.0f;

        // "Skyline" morph: lit windows reach 60% of all windows at City size 100 (D-15)
        constexpr double maxLitFraction = 0.6;

        // "Skyline" morph (D-27): time of day, from midday at 0 to dusk at 100, in keyframes at 0, 25, 50, 75 and 100
        constexpr int keyframeCount = 5;

        // "Skyline" window glow: the lit windows again, blurred by 3 pt, at 80%
        constexpr float glowBlurPoints = 3.0f;
        constexpr float glowAlpha = 0.8f;

        // "Skyline" bypassed: the greyscale of the current blend and the lit windows at 30% over the LCD base
        constexpr float bypassedOpacity = 0.3f;

        // "Skyline" bypassed and PRD §7: the two looks crossfade over 200 ms, eased in and out
        constexpr double bypassFadeMs = 200.0;
    }

    namespace BypassSwitch
    {
        // "Draw order" layer 8: the state images are 136 wide so the LED glow fits; they share the switch origin
        inline const juce::Rectangle<int> imageBounds { 608, 188, 136, 112 };

        // "Bypass switch" hit area: the 128-wide switch, including both labels, but not the glow overflow
        constexpr int hitWidth = 128;

        // "Draw order" layer 9: the track's hover and press overlay, switch-local; a pill, so the radius is half its width
        inline const juce::Rectangle<float> trackOverlay { 0.0f, 0.0f, 48.0f, 112.0f };
        constexpr float trackCornerRadius = 24.0f;
    }

    namespace Readout
    {
        // "Readout": the hit area; the LCD sits inside it
        inline const juce::Rectangle<int> hitArea { 60, 328, 120, 48 };
        inline const juce::Rectangle<float> lcdBounds { 68.0f, 332.0f, 104.0f, 40.0f };
        constexpr float lcdCornerRadius = 6.0f;

        // "Readout" editing: the selection covers the whole value, half digit included (readout-local), and a
        // 2 pt focus ring sits outside the LCD, so its outer radius is the LCD's plus 2
        inline const juce::Rectangle<float> selection { 24.0f, 8.0f, 82.0f, 32.0f };
        constexpr float focusRingThickness = 2.0f;

        // "Readout" geometry, the exact constants from the Figma script. Cell-local points: each cell is
        // 16 wide × 28 tall; ht is half the 4.5 segment thickness; L/R are the vertical segments' centre
        // lines and T/M/B the horizontal ones'.
        constexpr float cellTopY = 6.0f;
        constexpr float cellHeight = 28.0f;
        constexpr float segmentHalfThickness = 2.25f;
        constexpr float segmentGap = 1.0f;
        constexpr float segmentLeftX = 2.25f;
        constexpr float segmentRightX = 13.75f;
        constexpr float segmentTopY = 2.25f;
        constexpr float segmentMiddleY = 14.0f;
        constexpr float segmentBottomY = 25.75f;

        // Italic skew, pivoting on each cell's bottom edge
        constexpr float skewDegrees = 6.0f;

        // Cell x positions, LCD-local; the half digit shows only segments b and c
        constexpr float halfDigitCellX = 18.0f;
        constexpr float tensCellX = 36.0f;
        constexpr float onesCellX = 55.0f;
        constexpr float tenthsCellX = 80.0f;

        // An unskewed square, LCD-local
        inline const juce::Rectangle<float> decimalPoint { 74.5f, 30.0f, 4.0f, 4.0f };
    }
}
