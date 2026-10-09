#include "KnobPointer.h"
#include "Tokens.h"
#include "UiSpec.h"

namespace
{
    // The pointer at 12 o'clock, before rotation.
    juce::Rectangle<float> uprightBounds()
    {
        using namespace UiSpec::Knob;
        return { centre.x - pointerWidth * 0.5f,
                 centre.y - pointerInnerEndFromCentre - pointerLength,
                 pointerWidth,
                 pointerLength };
    }

    juce::AffineTransform rotationAbout (float angleRadians)
    {
        return juce::AffineTransform::rotation (angleRadians, UiSpec::Knob::centre.x, UiSpec::Knob::centre.y);
    }

    // Figma's inset 0 1 0 0: the pointer minus a copy of itself moved 1 pt towards the centre leaves a thin
    // edge at the outer end. Filled even-odd, so it still needs clipping to the pointer.
    juce::Path highlightAt (float angleRadians)
    {
        const auto bounds = uprightBounds();
        juce::Path highlight;
        highlight.setUsingNonZeroWinding (false);
        highlight.addRoundedRectangle (bounds, UiSpec::Knob::pointerCornerRadius);
        highlight.addRoundedRectangle (bounds.translated (0.0f, UiSpec::Knob::pointerHighlightThickness),
                                       UiSpec::Knob::pointerCornerRadius);
        highlight.applyTransform (rotationAbout (angleRadians));
        return highlight;
    }
}

juce::Path KnobPointer::outlineAt (float angleRadians)
{
    juce::Path outline;
    outline.addRoundedRectangle (uprightBounds(), UiSpec::Knob::pointerCornerRadius);
    outline.applyTransform (rotationAbout (angleRadians));
    return outline;
}

void KnobPointer::draw (juce::Graphics& g, float angleRadians)
{
    const auto outline = outlineAt (angleRadians);

    // One layer for fill and highlight, so the opacity applies to the pointer as a whole and the fill
    // doesn't show through the highlight.
    g.beginTransparencyLayer (Tokens::KnobPointer::opacity);

    g.setColour (juce::Colour (Tokens::Colours::inkPrimary));
    g.fillPath (outline);

    {
        const juce::Graphics::ScopedSaveState clipToPointer (g);
        g.reduceClipRegion (outline);
        g.setColour (juce::Colour (Tokens::Colours::knobPointerHighlight).withAlpha (Tokens::KnobPointer::highlightAlpha));
        g.fillPath (highlightAt (angleRadians));
    }

    g.endTransparencyLayer();
}
