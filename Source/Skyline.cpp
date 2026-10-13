#include "Skyline.h"
#include "ReduceMotion.h"
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
      bypassFade (initiallyBypassed),
      look (lookFor (initialCitySizePercent, initiallyBypassed ? 1.0f : 0.0f))
{
    // A display only: clicks fall through to the editor.
    setInterceptsMouseClicks (false, false);
    setBounds (UiSpec::Skyline::screenBounds);
    rebuildLitLayer();
}

void Skyline::setState (float citySizePercent, bool bypassed)
{
    updateShimmer (citySizePercent, bypassed);

    const auto nowSeconds = juce::Time::getMillisecondCounterHiRes() / 1000.0;
    const auto newLook = lookFor (citySizePercent, bypassFade.advance (bypassed, nowSeconds));

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
    // A shimmer frame repaints only the water. Mid-fade the normal look is a transparency layer that the
    // cached strip can't reproduce, so the fade always takes the full path below.
    if (look.bypassedShare <= 0.0f && waterBounds().contains (g.getClipBounds()))
    {
        paintWaterOnly (g);
        return;
    }

    juce::Path screen;
    screen.addRoundedRectangle (getLocalBounds().toFloat(), UiSpec::Skyline::screenCornerRadius);
    g.reduceClipRegion (screen);

    // Pixel art: nearest neighbour only, so no window or roofline ever smears.
    g.setImageResamplingQuality (juce::Graphics::lowResamplingQuality);

    if (look.bypassedShare > 0.0f)
        drawBypassed (g);

    if (look.bypassedShare < 1.0f)
        drawNormal (g, 1.0f - look.bypassedShare);

    // The glass isn't pixel art: its fine scanlines need smooth scaling on a 1x display.
    g.setImageResamplingQuality (juce::Graphics::mediumResamplingQuality);
    g.drawImage (overlay, getLocalBounds().toFloat());
}

void Skyline::updateShimmer (float citySizePercent, bool bypassed)
{
    shimmerCitySizePercent = citySizePercent;

    if (bypassed)
    {
        stopTimer();
        return;
    }

    if (citySizePercent > 0.0f && isShowing() && ! ReduceMotion::isOn())
    {
        if (! isTimerRunning())
            startTimerHz (UiSpec::Skyline::shimmerRateHz);

        return;
    }

    // At 0 this is the empty frame, so the water matches the keyframes exactly.
    stopTimer();
    showShimmer (WaterShimmer::stillFrame (citySizePercent));
}

void Skyline::timerCallback()
{
    showShimmer (WaterShimmer::frameAt (juce::Time::getMillisecondCounterHiRes() / 1000.0, shimmerCitySizePercent));
}

void Skyline::showShimmer (const WaterShimmer::Frame& frame)
{
    if (frame == shimmer)
        return;

    shimmer = frame;
    repaint (waterBounds());
}

Skyline::Look Skyline::lookFor (float citySizePercent, float bypassedShare) const
{
    return { SkylineMorph::litWindowCount (citySizePercent, (int) windows.size()),
             SkylineMorph::keyframeBlendFor (citySizePercent),
             bypassedShare };
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

void Skyline::drawNormal (juce::Graphics& g, float opacity) const
{
    // Mid-fade, the normal look goes over the bypassed one as a single layer. It's opaque, so this is an
    // exact crossfade between the two looks.
    const auto fading = opacity < 1.0f;

    if (fading)
        g.beginTransparencyLayer (opacity);

    drawNormalLook (g);
    drawShimmer (g);

    if (fading)
        g.endTransparencyLayer();
}

void Skyline::drawNormalLook (juce::Graphics& g) const
{
    drawKeyframeBlend (g, keyframes->colour);
    g.drawImage (litLayer, getLocalBounds().toFloat());
}

void Skyline::paintWaterOnly (juce::Graphics& g)
{
    const auto pixelScale = g.getInternalContext().getPhysicalPixelScaleFactor();

    if (! (waterStrip.look == look) || ! juce::exactlyEqual (waterStrip.pixelScale, pixelScale))
        rebuildWaterStrip (pixelScale);

    // Each strip pixel is one display pixel, so this is a straight copy.
    const auto water = waterBounds().toFloat();
    g.setImageResamplingQuality (juce::Graphics::lowResamplingQuality);
    g.drawImage (waterStrip.below, water);
    drawShimmer (g);
    g.drawImage (waterStrip.glass, water);
}

void Skyline::rebuildWaterStrip (float pixelScale)
{
    // Drawn with the same calls and scale as paint(), so the strip matches the full path pixel for pixel.
    const auto water = waterBounds();
    const auto toStrip = juce::AffineTransform::translation (0.0f, (float) -water.getY()).scaled (pixelScale);
    const auto width = juce::roundToInt ((float) water.getWidth() * pixelScale);
    const auto height = juce::roundToInt ((float) water.getHeight() * pixelScale);

    waterStrip.below = juce::Image (juce::Image::ARGB, width, height, true);
    {
        juce::Graphics below (waterStrip.below);
        below.addTransform (toStrip);
        below.setImageResamplingQuality (juce::Graphics::lowResamplingQuality);
        drawNormalLook (below);
    }

    // The glass never changes, so it's only redrawn when the display's scale does.
    if (! juce::exactlyEqual (waterStrip.pixelScale, pixelScale))
    {
        waterStrip.glass = juce::Image (juce::Image::ARGB, width, height, true);
        juce::Graphics glass (waterStrip.glass);
        glass.addTransform (toStrip);
        glass.setImageResamplingQuality (juce::Graphics::mediumResamplingQuality);
        glass.drawImage (overlay, getLocalBounds().toFloat());
    }

    waterStrip.look = look;
    waterStrip.pixelScale = pixelScale;
}

juce::Rectangle<int> Skyline::waterBounds() const
{
    return pointsFor (WaterShimmer::waterCells()).toNearestInt();
}

void Skyline::drawBypassed (juce::Graphics& g) const
{
    // One layer, so the grey windows don't show the grey skyline through them.
    g.beginTransparencyLayer (UiSpec::Skyline::bypassedOpacity);

    drawKeyframeBlend (g, keyframes->grey);
    g.setColour (juce::Colour (Tokens::Colours::skylineWindowLitBypassed));

    for (const auto window : std::span (windows).first ((size_t) look.litWindowCount))
        g.fillRect (pointsFor ({ window.x, window.y, 1, 1 }));

    g.endTransparencyLayer();
}

void Skyline::drawShimmer (juce::Graphics& g) const
{
    const juce::Colour highlight (Tokens::Colours::skylineWaterHighlight);

    if (shimmer.glintAlpha > 0.0f)
    {
        g.setColour (highlight.withAlpha (shimmer.glintAlpha));

        for (size_t glint = 0; glint < WaterShimmer::glintCount; ++glint)
            g.fillRect (pointsFor (WaterShimmer::glintCells (glint, shimmer.glintOffsets[glint])));
    }

    for (const auto& sparkle : std::span (shimmer.sparkles).first ((size_t) shimmer.sparkleCount))
    {
        g.setColour (highlight.withAlpha (sparkle.alpha));
        g.fillRect (pointsFor ({ sparkle.cell.x, sparkle.cell.y, 1, 1 }));
    }
}

juce::Rectangle<float> Skyline::pointsFor (juce::Rectangle<int> gridCells)
{
    return gridCells.toFloat() * UiSpec::Skyline::pointsPerGridPixel;
}
