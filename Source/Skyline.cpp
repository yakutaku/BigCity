#include "Skyline.h"
#include "SkylineMorph.h"
#include "Tokens.h"
#include "UiSpec.h"

#include <BinaryData.h>

#include <span>

namespace
{
    juce::Image loadImage (const char* data, int size)
    {
        return juce::ImageCache::getFromMemory (data, size);
    }

    // How much of a window of this width, starting at 0, a Gaussian blur spreads onto the pixel at x:
    // the blur of a box, worked out exactly rather than by convolution.
    float blurredBoxCoverage (float x, float windowWidth, float sigma)
    {
        const auto scale = 1.0f / (sigma * juce::MathConstants<float>::sqrt2);
        return 0.5f * (std::erf (x * scale) - std::erf ((x - windowWidth) * scale));
    }

    // The grey Figma's saturation blend leaves at full strength: the colour's luminosity with no hue
    // (Lum() in the W3C compositing spec, which Figma follows).
    juce::Colour withoutSaturation (juce::Colour colour)
    {
        constexpr float redWeight = 0.3f, greenWeight = 0.59f, blueWeight = 0.11f;
        const auto luminosity = redWeight * colour.getFloatRed() + greenWeight * colour.getFloatGreen()
                              + blueWeight * colour.getFloatBlue();
        return juce::Colour::fromFloatRGBA (luminosity, luminosity, luminosity, colour.getFloatAlpha());
    }

    // One window's glow, in image pixels. The window itself sits at (margin, margin).
    struct GlowSprite
    {
        static int marginPixels (int pixelsPerPoint)
        {
            return (int) std::ceil (3.0f * sigmaPixels (pixelsPerPoint));
        }

        // Assumes Figma's layer blur value is twice the Gaussian's standard deviation. Checked Day 8 against
        // a Figma screenshot at City size 50: the halo 1–3 pt out from each window matches.
        static float sigmaPixels (int pixelsPerPoint)
        {
            return UiSpec::Skyline::glowBlurPoints * 0.5f * (float) pixelsPerPoint;
        }

        static juce::Image create (int pixelsPerPoint, juce::Colour lit)
        {
            const auto windowPixels = UiSpec::Skyline::pointsPerGridPixel * (float) pixelsPerPoint;
            const auto margin = marginPixels (pixelsPerPoint);
            const auto size = (int) windowPixels + 2 * margin;
            const auto sigma = sigmaPixels (pixelsPerPoint);

            // A blurred square is the product of two blurred edges, so one row of coverage serves both axes.
            std::vector<float> coverage ((size_t) size);

            for (int i = 0; i < size; ++i)
                coverage[(size_t) i] = blurredBoxCoverage ((float) (i - margin) + 0.5f, windowPixels, sigma);

            juce::Image sprite (juce::Image::ARGB, size, size, true);

            for (int y = 0; y < size; ++y)
                for (int x = 0; x < size; ++x)
                    sprite.setPixelAt (x, y, lit.withAlpha (UiSpec::Skyline::glowAlpha * coverage[(size_t) x] * coverage[(size_t) y]));

            return sprite;
        }
    };
}

Skyline::Skyline (float initialCitySizePercent, bool initiallyBypassed)
    : colour (loadImage (BinaryData::skyline_colour_png, BinaryData::skyline_colour_pngSize)),
      grey (loadImage (BinaryData::skyline_grey_png, BinaryData::skyline_grey_pngSize)),
      overlay (loadImage (BinaryData::screen_overlay_png, BinaryData::screen_overlay_pngSize)),
      windows (SkylineMorph::parseWindows (juce::String::fromUTF8 (BinaryData::skylinewindows_json,
                                                                   BinaryData::skylinewindows_jsonSize))),
      pixelsPerPoint (colour.getWidth() / UiSpec::Skyline::screenBounds.getWidth()),
      litLayer (createLitLayer (juce::Colour (Tokens::Colours::skylineWindowLit))),
      greyLitLayer (createLitLayer (withoutSaturation (juce::Colour (Tokens::Colours::skylineWindowLit)))),
      look (lookFor (initialCitySizePercent, initiallyBypassed))
{
    // A display only: clicks fall through to the editor.
    setInterceptsMouseClicks (false, false);
    setBounds (UiSpec::Skyline::screenBounds);
    rebuildLitLayers();
}

void Skyline::setState (float citySizePercent, bool bypassed)
{
    const auto newLook = lookFor (citySizePercent, bypassed);

    if (newLook == look)
        return;

    const auto litCountChanged = newLook.litWindowCount != look.litWindowCount;
    look = newLook;

    if (litCountChanged)
        rebuildLitLayers();

    repaint();
}

void Skyline::paint (juce::Graphics& g)
{
    juce::Path screen;
    screen.addRoundedRectangle (getLocalBounds().toFloat(), UiSpec::Skyline::screenCornerRadius);
    g.reduceClipRegion (screen);

    // Pixel art: nearest neighbour only, so no window or roofline ever smears.
    g.setImageResamplingQuality (juce::Graphics::lowResamplingQuality);

    if (look.bypassed)
        drawBypassed (g);
    else
        drawNormal (g);

    // The glass isn't pixel art: its fine scanlines need smooth scaling on a 1x display.
    g.setImageResamplingQuality (juce::Graphics::mediumResamplingQuality);
    g.drawImage (overlay, getLocalBounds().toFloat());
}

Skyline::Look Skyline::lookFor (float citySizePercent, bool bypassed) const
{
    return { SkylineMorph::litWindowCount (citySizePercent, (int) windows.size()),
             SkylineMorph::greyLayerAlpha (citySizePercent),
             bypassed };
}

Skyline::LitLayer Skyline::createLitLayer (juce::Colour windowColour) const
{
    return { windowColour,
             GlowSprite::create (pixelsPerPoint, windowColour),
             juce::Image (juce::Image::ARGB, colour.getWidth(), colour.getHeight(), true) };
}

void Skyline::rebuildLitLayers()
{
    rebuild (litLayer);
    rebuild (greyLitLayer);
}

void Skyline::rebuild (LitLayer& layer) const
{
    // Drawn in the image's own pixels, so every window and glow lands on whole pixels with no resampling.
    layer.image.clear (layer.image.getBounds());
    juce::Graphics g (layer.image);
    const auto windowPixels = (int) UiSpec::Skyline::pointsPerGridPixel * pixelsPerPoint;
    const auto margin = GlowSprite::marginPixels (pixelsPerPoint);
    const auto lit = std::span (windows).first ((size_t) look.litWindowCount);

    g.setColour (layer.windowColour);

    for (const auto window : lit)
        g.fillRect (window.x * windowPixels, window.y * windowPixels, windowPixels, windowPixels);

    // The glow goes on top of the windows, as in Figma.
    for (const auto window : lit)
        g.drawImageAt (layer.glowSprite, window.x * windowPixels - margin, window.y * windowPixels - margin);
}

void Skyline::drawNormal (juce::Graphics& g) const
{
    const auto bounds = getLocalBounds().toFloat();

    g.drawImage (colour, bounds);

    g.setOpacity (look.greyLayerAlpha);
    g.drawImage (grey, bounds);
    g.setOpacity (1.0f);

    g.drawImage (litLayer.image, bounds);

    g.setOpacity (look.greyLayerAlpha);
    g.drawImage (greyLitLayer.image, bounds);
}

void Skyline::drawBypassed (juce::Graphics& g) const
{
    // One layer, so the grey windows don't show the grey skyline through them.
    g.beginTransparencyLayer (UiSpec::Skyline::bypassedOpacity);

    g.drawImage (grey, getLocalBounds().toFloat());
    g.setColour (juce::Colour (Tokens::Colours::skylineWindowLitBypassed));

    for (const auto window : std::span (windows).first ((size_t) look.litWindowCount))
        g.fillRect (windowBounds (window));

    g.endTransparencyLayer();
}

juce::Rectangle<float> Skyline::windowBounds (juce::Point<int> window) const
{
    const auto size = UiSpec::Skyline::pointsPerGridPixel;
    return { (float) window.x * size, (float) window.y * size, size, size };
}
