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

    // "Performance": repaint timer
    constexpr int repaintRateHz = 30;

    namespace Knob
    {
        // "Draw order" layers 4 and 5: knob_base and knob_rotor share these bounds, the knob's origin minus 8
        inline const juce::Rectangle<int> imageBounds { 24, 136, 192, 192 };

        // "Knob": the hit area is the 176 × 176 knob bounds, inset this far inside the images
        constexpr int hitAreaInset = 8;

        // "Draw order" layer 5: the rotor's pivot, image-local
        inline const juce::Point<float> rotorPivot { 96.0f, 96.0f };

        // "Draw order" layer 6: the hover and press ellipse, relative to the knob's origin (not the images')
        inline const juce::Rectangle<float> overlayEllipse { 32.0f, 32.0f, 112.0f, 112.0f };

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
    }

    namespace Readout
    {
        // "Readout": the hit area; the LCD sits inside it
        inline const juce::Rectangle<int> hitArea { 60, 328, 120, 48 };
        inline const juce::Point<float> lcdOrigin { 68.0f, 332.0f };

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
