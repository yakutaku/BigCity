#pragma once

#include <juce_core/juce_core.h>

// Design tokens, mirrored from docs/ui-spec.md "Tokens used in code" and the Practical UI starter kit (CLAUDE.md).
// This is the only file where a hex value or a spacing size may be written.
namespace Tokens
{
    // 0xAARRGGBB, so the tokens stay constexpr; wrap them in juce::Colour where they're drawn.
    namespace Colours
    {
        constexpr juce::uint32 inkPrimary = 0xff3b3160;                 // ink/primary
        constexpr juce::uint32 inkSecondary = 0xff4f4577;               // ink/secondary
        constexpr juce::uint32 accentTeal = 0xff1f6f68;                 // accent/teal
        constexpr juce::uint32 strokeStrong = 0xff676080;               // stroke/strong
        constexpr juce::uint32 pastelSky = 0xffa9d3ee;                  // pastel/sky
        constexpr juce::uint32 surfaceLcd = 0xffd3dcc0;                 // surface/lcd
        constexpr juce::uint32 skylineWindowLit = 0xfffbe08a;           // skyline/windowLit
        constexpr juce::uint32 skylineWindowLitBypassed = 0xffe0e0e0;   // skyline/windowLitBypassed

        constexpr juce::uint32 knobPointerHighlight = 0xfffffdf6;       // knob/pointerHighlight

        // docs/practical-ui.md "Hover and press use transparent overlays": hover is white
        constexpr juce::uint32 hoverOverlayBase = 0xffffffff;
    }

    // docs/practical-ui.md "Hover and press use transparent overlays"
    namespace Overlay
    {
        constexpr float hoverAlpha = 0.10f;  // over Colours::hoverOverlayBase
        constexpr float pressAlpha = 0.08f;  // over Colours::inkPrimary
    }

    // docs/ui-spec.md "Knob" pointer, from Figma
    namespace KnobPointer
    {
        constexpr float opacity = 0.92f;          // the whole pointer, fill and highlight together
        constexpr float highlightAlpha = 0.25f;   // over Colours::knobPointerHighlight
    }

    // The 8 pt grid. tight4 and tight12 are only for spacing inside a component.
    namespace Spacing
    {
        constexpr int tight4 = 4;
        constexpr int tight12 = 12;
        constexpr int xs = 8;
        constexpr int s = 16;
        constexpr int m = 24;
        constexpr int l = 32;
        constexpr int xl = 48;
        constexpr int xxl = 80;
        constexpr int xxxl = 96;
    }
}
