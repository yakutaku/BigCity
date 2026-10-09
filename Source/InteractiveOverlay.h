#pragma once

#include "Tokens.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <optional>

// docs/practical-ui.md "Hover and press use transparent overlays": the colour a control lays over itself
// for its current mouse state, or nothing when it's idle.
namespace InteractiveOverlay
{
    inline std::optional<juce::Colour> colourFor (const juce::Component& control)
    {
        if (control.isMouseButtonDown())
            return juce::Colour (Tokens::Colours::inkPrimary).withAlpha (Tokens::Overlay::pressAlpha);

        if (control.isMouseOver())
            return juce::Colour (Tokens::Colours::hoverOverlayBase).withAlpha (Tokens::Overlay::hoverAlpha);

        return std::nullopt;
    }
}
