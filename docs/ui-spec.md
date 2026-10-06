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

## Draw order (back to front)

| # | Layer | Source | Position (x, y, w, h) | Repaints when |
|---|---|---|---|---|
| 1 | `panel.png` | Static image: background, aging, header (wordmark, サンシャワー・オーディオ with the marker doodle, model block), stripes, screws, CITY SIZE label, readout LCD with burned-in 188.8, bezel, LCD screen base | 0, 0, 768, 416 | Never (cache it) |
| 2 | Skyline | `skyline_colour.png` + `skyline_grey.png` + lit windows | 240, 104, 336, 280 | Value or bypass changes |
| 3 | `screen_overlay.png` | Static image: scanlines, glare, glass scratches | 240, 104, 336, 280 (corner radius 14) | Never |
| 4 | `knob_base.png` | Static image: ticks, arc, cast shadow, skirt and face lighting | 24, 136, 192, 192 (the knob's origin minus 8) | Never |
| 5 | `knob_rotor.png` | Rotated image: knurl, grime, rim wear, face wear, pointer | Same as `knob_base`; pivot at (96, 96) in the image, which is (120, 232) in the window | Value changes |
| 6 | Knob overlay | Code: an ellipse at 32, 32, 112, 112 relative to the knob origin (32, 144) | | Hover and press |
| 7 | Readout digits | Code: 7-segment paths (see below), drawn in the LCD at 68, 332, 104, 40 | | Value changes |
| 8 | `switch_fx_on.png` or `switch_bypassed.png` | One image per state | 608, 188, 128, 112 | Bypass changes |
| 9 | Switch overlay | Code: a rounded rect at 0, 0, 48, 112 (radius 24) relative to the switch | | Hover and press |

## Knob
- **Angle:** `angle = −135 + 2.7 × value` degrees, clockwise from 12 o'clock. Only `knob_rotor` rotates; the lighting in `knob_base` stays fixed.
- **Ticks:** 11 ticks, one per 10%. The ticks at 0, 50 and 100 are major (2 pt); the rest are 1.5 pt. They are drawn in `ink/secondary` and baked into `knob_base`.
- **Hit area:** the 176 × 176 knob bounds.
- **Overlays:**
  - Hover: white at 10%.
  - Press and drag: `ink/primary` at 8%.
- **Cursor:** vertical double arrow.
- **Input** (PRD §7):
  - Drag: about 200 px covers the full range.
  - Shift + drag: fine control in 0.1 steps.
  - Scroll wheel or trackpad: changes the value; hold Shift for fine control.
  - Option-click: resets to 50.
  - Double-click: opens text entry.

## Readout: 7-segment LCD (D-21, approved; replaces D2)
- **Hit area:** 60, 328, 120, 48. The LCD and its burned-in "188.8" (all segments at 13% `ink/primary`) are baked into `panel.png`.
- **Digits:**
  - 3½ digits: a half-digit "1" for 100.0, then tens, ones and tenths. Leading zeros are blank, e.g. " 5.0".
  - Lit segments are drawn as paths in `ink/primary`. **No font is needed.**
- **Geometry** (LCD-local points; mirror the constants from the Figma script):
  - Each cell is 15 × 28 at y 6; segment thickness 3 with hexagonal ends; 1.1 gap between segments; italic skew of 6°.
  - Cell x positions: half digit 21, tens 39, ones 57, tenths 79.
  - Decimal point: a 3 × 3 square at (74, 34).
  - Segment map: 0 abcdef · 1 bc · 2 abged · 3 abgcd · 4 fgbc · 5 afgcd · 6 afgedc · 7 abc · 8 abcdefg · 9 abcdfg.
- **Editing:**
  - Double-click opens entry with the value selected. The selection uses `pastel/sky` at 44, 8, 62, 32 in readout-local coordinates.
  - Typed characters render as segments.
  - The LCD gets a 2 pt `accent/teal` focus ring outside its bounds, and the burned-in ghost is hidden.
  - Enter commits and clamps to 0–100. Esc cancels.
- **Bypassed:** unchanged. Full contrast and editable (D-10).

## Bypass switch
- **Size:** 128 × 112.
- **Labels:** FX ON and BYPASS share one left edge at x 60. The LED sits inline, 8 pt after FX ON.
- **Hit area:** the whole switch, including both labels.
- **Cursor:** pointing hand.
- **Overlays on the track:**
  - Hover: white at 10%.
  - Press: `ink/primary` at 8%.
- **Toggling:** happens on mouse up. The switch is bound to the host bypass parameter (`getBypassParameter()`), so automation moves it too.

## Skyline
- **Grid:** 112 × 95 logical pixels at 3 pt each, so 6 px @2x. The screen shows the top 280 pt. Scale with **nearest neighbour** only.
- **Generator:** `tools/skyline/skyline_gen.js` is the single source, and it is the same code that drew the Figma layers. To regenerate the window order, run `node tools/skyline/skyline_gen.js assets/skyline-windows.json`.

**Morph** (driven by the smoothed City size `s`, 0–100):

| Element | Rule |
|---|---|
| Base | Draw `skyline_colour.png`. |
| Saturation (70% → 100%) | Draw `skyline_grey.png` on top at alpha `0.3 × (1 − s/100)`. |
| Lit windows | Light the first `floor(353 × 0.6 × s/100)` entries of `assets/skyline-windows.json`. Each is a 3 × 3 pt square at screen-local `(x × 3, y × 3)`, colour `#FBE08A`. |
| Window glow | Draw the lit windows again, blurred by 3 pt at 80%, on top. Cache it per lit count. |

Reference counts:

| City size | 0 | 25 | 50 | 75 | 100 |
|---|---|---|---|---|---|
| Windows lit | 0 | 52 | 105 | 158 | 211 |

**Bypassed:**
- Draw `skyline_grey.png` plus the lit windows in `#E0E0E0`, at 30% opacity, over the LCD base.
- Fade over 200 ms with ease-in-out. If time runs short, cut #1 replaces the fade with an instant switch.

## Performance
- Cache layers 1, 3 and 4.
- Repaint only the screen rect on value changes and the switch rect on bypass changes, with a timer of at most 30 fps.

## Fonts
- **None.** Every label, including the Japanese, is baked into the images (D-18), and the readout is drawn as segments (D-21).

## Tokens used in code
| Token | Value |
|---|---|
| `ink/primary` | `#3B3160` |
| `ink/secondary` | **`#4F4577`** (changed from `#5E5486`) |
| `accent/teal` | `#1F6F68` |
| `pastel/sky` | `#A9D3EE` |
| `surface/lcd` | `#D3DCC0` |
| `skyline/windowLit` | `#FBE08A` |
| `skyline/windowLitBypassed` | `#E0E0E0` |

## Assets checklist
Export from Figma: select `Export · JUCE assets @2x` on 02 Plugin, then Export.

| File | Size (px) |
|---|---|
| `panel.png` | 1536×832 |
| `skyline_colour.png` | 672×560 |
| `skyline_grey.png` | 672×560 |
| `screen_overlay.png` | 672×560 |
| `knob_base.png` | 384×384 |
| `knob_rotor.png` | 384×384 |
| `switch_fx_on.png` | 256×224 |
| `switch_bypassed.png` | 256×224 |

Also needed: `assets/skyline-windows.json`.
