# BigCity UI build spec (v4 · design freeze, Day 6 · 10/6)

The source is the Figma file `PH0gcr5T6dAacpPSCB8FWw`:
- Page **02 Plugin:** hero frame `5:71` ("BigCity v4") and the export sheet `Export · JUCE assets @2x`
- Page **03 States:** every state, in frame "BigCity v4 · States"

All coordinates are in points, relative to the window's top-left. Assets are exported at @2x.

## Window and layout
- Fixed size of **768 × 416** (D-11). Not resizable.
- **Horizontal:** 32 margin | knob group 176 | 24 | screen bezel 352 | 24 | switch 128 | 32.
- **Vertical:**
  - Header band: 0–56, with content centred on y 28.
  - Stripes: 57–72.
  - Body: 72–416. The bezel sits at 96–392, with 24 above and below.
- **Shared centre line:** the knob group (label, knob, readout; 264 tall) and the switch are both centred on the screen's centre line at **y 244**.
- **Spacing inside groups:**
  - label → knob: 8
  - knob → readout: 8
  - switch track → labels: 12
  - label → LED: 8
- Screws are at an 8 inset; each one is turned to a different angle.
- The switch image is 136 wide; only the LED glow enters the right margin. The switch and its hit area end at 736.

## Draw order (back to front)

| # | Layer | Source | Position (x, y, w, h) | Repaints when |
|---|---|---|---|---|
| 1 | `panel.png` | Static image: background, aging, header (wordmark, サンシャワー・オーディオ with the marker doodle, model block), stripes, screws, CITY SIZE label, readout LCD with burned-in 188.8, bezel, LCD screen base | 0, 0, 768, 416 | Never (cache it) |
| 2 | Skyline | Crossfade of two `skyline_tod_*.png` keyframes + lit windows | 240, 104, 336, 280 | Value or bypass changes |
| 3 | `screen_overlay.png` | Static image: scanlines, glare, glass scratches | 240, 104, 336, 280 (corner radius 14) | Never |
| 4 | `knob_base.png` | Static image: ticks, arc, cast shadow, skirt and face lighting | 24, 136, 192, 192 (the knob's origin minus 8) | Never (cached with the panel behind it as one image) |
| 5 | `knob_rotor.png` | Rotated image: knurl, grime, rim wear, face wear. **No pointer** (D-25) | Same as `knob_base`; pivot at (96, 96) in the image, which is (120, 232) in the window | Value changes |
| 5b | Knob pointer | Code: a vector shape rotated with the rotor (see "Knob") | | Value changes |
| 6 | Knob overlay | Code: the cap circle, an ellipse at 32, 32, 112, 112 relative to the knob origin (32, 144) | | Hover and press |
| 7 | Readout digits | Code: 7-segment paths (see below), drawn in the LCD at 68, 332, 104, 40 | | Value changes |
| 8 | `switch_fx_on.png` or `switch_bypassed.png` | One image per state | 608, 188, 136, 112 (the LED glow overflows the 128-wide switch) | Bypass changes |
| 9 | Switch overlay | Code: a rounded rect at 0, 0, 48, 112 (radius 24) relative to the switch | | Hover and press |

## Knob
- **Angle:** `angle = −135 + 2.7 × value` degrees, clockwise from 12 o'clock. Only `knob_rotor` and the pointer rotate; the lighting in `knob_base` stays fixed.
- **Pointer** (Figma `I5:101;5:51`; D-25): drawn in code as an anti-aliased path, not baked into the rotor.
  - A 4 × 20 rounded rect, radius 2, at 12 o'clock with its inner end 30 above the centre (so 30–50 from it).
  - Fill `ink/primary`, plus a 1 pt inner highlight along the outer end in `knob/pointerHighlight` at 25% (Figma's inset `0 1 0 0`).
  - The fill and highlight are drawn together at 92% opacity.
- **Motion** (D-25): only the picture eases; the parameter and the readout never do.
  - Drags and trackpad swipes follow 1:1 on every frame, however fast.
  - Mouse-wheel notches and Option-click glide to the new angle over 80 ms with a cubic ease-out, `1 − (1 − t)³`.
  - Host changes glide the same way only when they move more than 1.0 in one display frame (an automation step, a preset). Smaller per-frame changes, such as automation curves, follow 1:1.
  - A new jump mid-glide restarts from where the knob is, so it never snaps.
- **Ticks:** 11 ticks, one per 10%. The ticks at 0, 50 and 100 are major (2 pt); the rest are 1.5 pt. They are drawn in `ink/secondary` and baked into `knob_base`.
- **Hit area:** the cap (the skirt): a circle of radius 56 around the knob centre (120, 232). Hover, press, the cursor, clicks and scrolling apply only inside it, not over the ticks or the rest of the 176 square. Its 112 diameter clears the 48 pt minimum.
- **Overlays:**
  - Hover: white at 10%.
  - Press and drag: `ink/primary` at 8%.
- **Cursor:** vertical double arrow.
- **Input** (PRD §7):
  - Drag: about 200 px covers the full range.
  - Shift + drag: fine control in 0.1 steps.
  - Scroll wheel or trackpad: changes the value; hold Shift for fine control.
    - Mouse wheel: 1.0 per notch, 0.1 with Shift.
    - Trackpad: moves as far as a drag of the same distance, after a small dead zone (8 px per scroll burst) so a light brush doesn't move the knob. A burst ends after 300 ms without scrolling, or on a click.
  - Option-click: resets to 50.
  - Double-click: opens text entry.

## Readout: 7-segment LCD (D-21, approved; replaces D2)
- **Hit area:** 60, 328, 120, 48. The LCD and its burned-in "188.8" (all segments at 13% `ink/primary`) are baked into `panel.png`.
- **Digits:**
  - 3½ digits: a half-digit "1" for 100.0, then tens, ones and tenths. Leading zeros are blank, e.g. " 5.0".
  - Lit segments are drawn as paths in `ink/primary`. **No font is needed.**
- **Geometry** (LCD-local points; mirror the constants from the Figma script):
  - Each cell is 16 × 28 at y 6; segment thickness 4.5 with hexagonal ends; 1.0 gap between segments; italic skew of 6°.
  - Cell x positions: half digit 18, tens 36, ones 55, tenths 80.
  - Decimal point: a 4 × 4 square at (74.5, 30).
  - Segment map: 0 abcdef · 1 bc · 2 abged · 3 abgcd · 4 fgbc · 5 afgcd · 6 afgedc · 7 abc · 8 abcdefg · 9 abcdfg.
- **Editing:**
  - Double-click (on the readout or the knob) opens entry with the value selected. The selection covers the whole value, half digit included: `pastel/sky` at 24, 8, 82, 32 in readout-local coordinates.
  - Accepts digits, one decimal point and Backspace, and only what the LCD can show: one decimal digit, and three integer digits only when the first is 1. Typed characters render as segments, right-aligned to the ones cell; the decimal point stays lit.
  - The LCD (corner radius 6) gets a 2 pt `accent/teal` focus ring outside its bounds, outer radius 8. The burned-in ghost stays visible, as on a real LCD.
  - Enter, or clicking anywhere else, commits and clamps to 0–100 in one automation gesture. Esc cancels. Empty or invalid input keeps the old value.
- **Bypassed:** unchanged. Full contrast and editable (D-10).

## Bypass switch
- **Size:** 128 × 112. The state images are 136 × 112 so the LED glow fits; they share the switch origin.
- **Labels:** FX ON and BYPASS share one left edge at x 60. The LED sits inline, 8 pt after FX ON.
- **Hit area:** 128 × 112 at 608, 188: the whole switch, including both labels, but not the glow overflow.
- **Cursor:** pointing hand.
- **Overlays on the track:**
  - Hover: white at 10%.
  - Press: `ink/primary` at 8%.
- **Toggling:** happens on mouse up. The switch is bound to the host bypass parameter (`getBypassParameter()`), so automation moves it too.

## Skyline
- **Grid:** 112 × 95 logical pixels at 3 pt each, so 6 px @2x. The screen shows the top 280 pt. Scale with **nearest neighbour** only.
- **Generator:** `tools/skyline/skyline_gen.js` is the single source, and it is the same code that drew the Figma layers. To regenerate the window order, run `node tools/skyline/skyline_gen.js assets/skyline-windows.json`.

**Morph** (driven by `s`, the knob's value on screen, 0–100, so the skyline and knob move as one; D-26). The knob is the time of day, from midday at 0 to dusk at 100 (D-27):

| Element | Rule |
|---|---|
| Time of day | Keyframes `skyline_tod_000/025/050/075/100.png` sit at s = 0, 25, 50, 75, 100. Draw the keyframe at or below `s`, then the next one on top at alpha `(s − its s) / 25`: a linear crossfade. At 100 the last keyframe shows alone. The keyframes are opaque. |
| Lit windows | Light the first `floor(353 × 0.6 × s/100)` entries of `assets/skyline-windows.json`. Each is a 3 × 3 pt square at screen-local `(x × 3, y × 3)`, colour `#FBE08A`. |
| Window glow | Draw the lit windows again, blurred by 3 pt at 80%, on top. Cache it per lit count. |

The window order still fits the keyframes: all 353 windows land on an unlit-window pixel in every keyframe (checked Day 10).

Reference counts:

| City size | 0 | 25 | 50 | 75 | 100 |
|---|---|---|---|---|---|
| Windows lit | 0 | 52 | 105 | 158 | 211 |

**Bypassed:**
- Draw the greyscale of the current crossfade plus the lit windows in `#E0E0E0`, at 30% opacity, over the LCD base.
- The grey keyframes are computed once at load, not shipped as assets: each pixel's luminosity, `0.3 R + 0.59 G + 0.11 B` (Figma's `Lum()`). Greyscale is linear, so crossfading the grey keyframes gives the greyscale of the colour crossfade.
- Fade over 200 ms with a cubic ease-in-out (`4t³` for the first half, `1 − (2 − 2t)³ / 2` for the second; built Day 10, so cut #1 isn't needed). The bypassed look is drawn first, then the opaque normal look over it at `1 − share`, so the two crossfade exactly. Toggling mid-fade turns back from where it is. The fade runs on the editor's 30 fps timer, so 200 ms is about 6 frames.
- The screen clips the skyline to a 14 pt corner radius (Practical UI exception D7).

## Performance
- Cache layers 1, 3 and 4. The knob caches the panel behind it and layer 4 as one image, so it can be opaque.
- Repaint only the screen rect on value changes and the switch image rect (608, 188, 136, 112) on bypass changes, with a timer of at most 30 fps.
- **Memory:** the 5 keyframes and their 5 grey copies (15.1 MB) are decoded once and shared by every open editor (`juce::SharedResourcePointer`). The first editor to open loads them and the last to close frees them. Each open editor adds its lit-window layer (1.5 MB) and the knob's cached static layers (0.6 MB). The panel, knob, switch and glass images (8.3 MB) are shared through `juce::ImageCache`, which drops them 5 s after the last editor closes.

  | Open editors | 0 | 1 | 3 |
  |---|---|---|---|
  | UI images | 0 MB | 25.4 MB | 29.6 MB |
- **Exception, the knob (D-25):** it reads the value on every display refresh (`juce::VBlankAttachment`) and repaints its own rect only when its angle moves. Idle costs one read and a comparison per refresh; nothing is drawn.

## Fonts
- **None.** Every label, including the Japanese, is baked into the images (D-18), and the readout is drawn as segments (D-21).

## Tokens used in code
The knob rim (`#706248`) and the LCD edge (`#5F6B49`) are baked into the images, so they need no token.

| Token | Value |
|---|---|
| `ink/primary` | `#3B3160` |
| `ink/secondary` | **`#4F4577`** (changed from `#5E5486`) |
| `accent/teal` | `#1F6F68` |
| `stroke/strong` | **`#676080`** (changed from `#7D7590`; control edges stay ≥3:1 on the aged panel) |
| `pastel/sky` | `#A9D3EE` |
| `surface/lcd` | `#D3DCC0` |
| `skyline/windowLit` | `#FBE08A` |
| `skyline/windowLitBypassed` | `#E0E0E0` |
| `knob/pointerHighlight` | `#FFFDF6`, used at 25% |

The pointer's 92% opacity is also a token (`Tokens::KnobPointer::opacity`).

## Assets checklist
Export from Figma: select `Export · JUCE assets @2x` on 02 Plugin, then Export.

| File | Size (px) |
|---|---|
| `panel.png` | 1536×832 |
| `skyline_tod_000.png` … `skyline_tod_100.png` (5 keyframes: 0, 25, 50, 75, 100) | 672×560 each |
| `screen_overlay.png` | 672×560 |
| `knob_base.png` | 384×384 |
| `knob_rotor.png` | 384×384, with the Pointer layer hidden (the pointer is drawn in code) |
| `switch_fx_on.png` | 272×224 |
| `switch_bypassed.png` | 272×224 |

Also needed: `assets/skyline-windows.json`.
