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
    : overlay (loadImage (BinaryData::screen_overlay_png, BinaryData::screen_overlay_pngSize)),
      windows (SkylineMorph::parseWindows (juce::String::fromUTF8 (BinaryData::skylinewindows_json,
                                                                   BinaryData::skylinewindows_jsonSize))),
      pixelsPerPoint (keyframes->colour.front().getWidth() / UiSpec::Skyline::screenBounds.getWidth()),
      glowSprite (GlowSprite::create (pixelsPerPoint, juce::Colour (Tokens::Colours::skylineWindowLit))),
      litLayer (juce::Image::ARGB, keyframes->colour.front().getWidth(), keyframes->colour.front().getHeight(), true),
      look (lookFor (initialCitySizePercent, initiallyBypassed))
{
    // A display only: clicks fall through to the editor.
    setInterceptsMouseClicks (false, false);
    setBounds (UiSpec::Skyline::screenBounds);
    rebuildLitLayer();
}

void Skyline::setState (float citySizePercent, bool bypassed)
{
    const auto newLook = lookFor (citySizePercent, bypassed);

    if (newLook == look)
        return;

    const auto litCountChanged = newLook.litWindowCount != look.litWindowCount;
    look = newLook;

    if (litCountChanged)
        rebuildLitLayer();

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
             SkylineMorph::keyframeBlendFor (citySizePercent),
             bypassed };
}

void Skyline::rebuildLitLayer()
{
    // Drawn in the image's own pixels, so every window and glow lands on whole pixels with no resampling.
    litLayer.clear (litLayer.getBounds());
    juce::Graphics g (litLayer);
    const auto windowPixels = (int) UiSpec::Skyline::pointsPerGridPixel * pixelsPerPoint;
    const auto margin = GlowSprite::marginPixels (pixelsPerPoint);
    const auto lit = std::span (windows).first ((size_t) look.litWindowCount);

    g.setColour (juce::Colour (Tokens::Colours::skylineWindowLit));

    for (const auto window : lit)
        g.fillRect (window.x * windowPixels, window.y * windowPixels, windowPixels, windowPixels);

    // The glow goes on top of the windows, as in Figma.
    for (const auto window : lit)
        g.drawImageAt (glowSprite, window.x * windowPixels - margin, window.y * windowPixels - margin);
}

void Skyline::drawKeyframeBlend (juce::Graphics& g, const SkylineKeyframes::Frames& frames) const
{
    const auto bounds = getLocalBounds().toFloat();
    const auto lower = (size_t) look.blend.lowerIndex;

    // The keyframes are opaque, so the upper one at this alpha over the lower is a linear crossfade.
    g.drawImage (frames[lower], bounds);

    if (look.blend.upperAlpha <= 0.0f)
        return;

    g.setOpacity (look.blend.upperAlpha);
    g.drawImage (frames[lower + 1], bounds);
    g.setOpacity (1.0f);
}

void Skyline::drawNormal (juce::Graphics& g) const
{
    drawKeyframeBlend (g, keyframes->colour);
    g.drawImage (litLayer, getLocalBounds().toFloat());
}

void Skyline::drawBypassed (juce::Graphics& g) const
{
    // One layer, so the grey windows don't show the grey skyline through them.
    g.beginTransparencyLayer (UiSpec::Skyline::bypassedOpacity);

    drawKeyframeBlend (g, keyframes->grey);
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
