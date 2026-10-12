# BigCity decision log

Each hard call, with its options, reasoning and evidence. This is the source for the case study. Add one entry the day a decision is made.

**Format:** context → options → decision → evidence → result.

---

## D-01 · One macro knob, not a dry/wet (Day 1)
- **Context:** the v1.0 PRD called the knob a dry/wet blend of the air and micro-shift effects.
- **Options:** (a) dry/wet; (b) a macro that scales several parameters along tuned curves.
- **Decision:** a macro. City size drives air gain, detune, shimmer level, and loudness compensation at once.
- **Evidence:** a dry/wet only blends two fixed sounds, so most positions are a compromise. A macro lets every position be a deliberately tuned sound.
- **Result:** 50 was approved by ear as a strong default, and 100 is the full effect.

## D-02 · Native JUCE UI, not a web view (Day 1)
- **Context:** the plugin's pitch is "light on CPU."
- **Options:** (a) a JUCE WebView UI (easier Figma-to-code with AI); (b) native JUCE drawing.
- **Decision:** native.
- **Evidence:** a WebView embeds a browser engine in every plugin instance, which contradicts the CPU promise.
- **Result:** tracked in the CPU benchmark (Day 9).

## D-03 · Cut scope early: no automation dot, no arrow keys (Day 1)
- **Decision:** cut both from v1.
- **Evidence:** Ableton already shows automation state itself. Plugin windows don't reliably receive keyboard focus in Ableton, so arrow keys would behave inconsistently. Scroll wheel and trackpad replace them.
- **Result:** less to build and nothing users lose.

## D-04 · Air shelf Q 0.707, chosen by A/B (Day 3)
- **Options:** Q 0.5 (gentle, broad) vs 0.707 (clean, no overshoot).
- **Decision:** 0.707.
- **Evidence:** A/B listen on a vocal and a full mix at City size 100. 0.707 had more top end without harshness.
- **Result:** locked in SoundSpec.h.

## D-05 · Accept a 0.73 dB sample-rate difference instead of oversampling (Day 3)
- **Context:** a standard digital shelf near 12 kHz behaves slightly differently at 44.1 kHz than at 96 kHz.
- **Measured:** at most 0.73 dB difference, only above 14.8 kHz (ShelfCheck tool).
- **Options:** (a) accept; (b) oversample (costs CPU); (c) a matched analog-style design (costs time).
- **Decision:** accept for v1; option (c) goes in the v1.1 backlog.
- **Evidence:** under 1 dB, in the least sensitive part of hearing, and only noticeable comparing one session at two sample rates.

## D-06 · Raise the minimum to macOS 12 (Day 2)
- **Context:** Xcode 27 can't build for macOS 11.
- **Options:** (a) macOS 12 minimum; (b) maintain a second, older Xcode.
- **Decision:** macOS 12.
- **Evidence:** a small user loss (macOS 12 has been out since 2021) vs. ongoing toolchain complexity and signing on an old toolchain.

## D-07 · Tune by ear with live sliders, grounded in reference gear (Day 4)
- **Context:** v1 sounded "airy but not enough sheen," then "artificial at max."
- **Method:** a developer-only tuning build with live sliders, so there was no rebuild per change. Every slider range was bounded by sourced data.
- **Sources:** Eventide H3000 micropitch practice (±7–10 cents, 6–20 ms); Juno-60 chorus (0.51/0.86 Hz, 1.66–5.35 ms sweep); Soundtoys MicroShift (time-varying pitch, a crossover so the effect stays above the lows, analog saturation).
- **Final:**
  - air +7.8–8.1 dB at 10 kHz
  - shimmer −3 dB at max (−9 dB at 50)
  - detune 5.5 / 7.7 / 11 cents
  - movement 1.65 ms at 0.72 Hz
  - wet band 250 Hz–7 kHz
  - wet drive 3 dB
- **Result:** "warmer, more luxurious, with movement." Max values were capped at the point where they turned artificial: depth before warble, rate as a ceiling.

## D-08 · Calibrate loudness on music, not pink noise (Day 4)
- **Context:** auto gain was calibrated on pink noise, but BigCity sounded slightly quieter than bypass on real material.
- **Cause:** pink noise has more high-frequency energy than music, so the air boost raised its measured loudness more than it raises a mix. The plugin then over-compensated.
- **Decision:** re-measure on IEC 60268-1 programme simulation noise (Clause 7, Table II), a standard noise whose spectrum matches the average of speech and music: flat from 250 to 800 Hz, −11.9 dB at 10 kHz. The first plan was a dry vocal and a dry full mix.
- **Evidence:** the old pink-noise curve, measured on programme simulation noise, left BigCity quieter than bypass at every position, by 0.98 dB at 50 and 2.16 dB at 100. Same method otherwise: −18 dBFS RMS, 48 kHz, BS.1770 K-weighted, stereo.
- **Result (10/5):** about a third of the old compensation at 100. 0 stays exactly 0 dB, so 0 still nulls against bypass. After the change, every point measures within 0.01 dB of bypass in stereo; mono is within 0.46 dB (at 100), inside the 0.5 dB tolerance.

  | City size | 0 | 10 | 20 | 30 | 40 | 50 | 60 | 70 | 80 | 90 | 100 |
  |---|---|---|---|---|---|---|---|---|---|---|---|
  | Old gain, pink noise (dB) | 0.00 | −0.20 | −0.43 | −0.69 | −0.99 | −1.33 | −1.67 | −2.04 | −2.45 | −2.90 | −3.38 |
  | Old gain vs bypass on programme noise (dB) | 0.00 | −0.18 | −0.36 | −0.54 | −0.75 | −0.98 | −1.20 | −1.44 | −1.68 | −1.93 | −2.16 |
  | **New gain, programme noise (dB)** | **0.00** | **−0.02** | **−0.07** | **−0.15** | **−0.24** | **−0.35** | **−0.47** | **−0.60** | **−0.77** | **−0.97** | **−1.22** |

## D-09 · Uppercase control labels as a documented Practical UI exception (Day 2)
- **Rule:** Practical UI says sentence case.
- **Decision:** uppercase for on-plugin control and header labels only.
- **Evidence:** this is the plugin convention (Valhalla, Serum) and the hardware reference. The labels are 1–2 words.
- **Guardrails:** Jost Semi Bold 14/24 with +2 letter spacing, 4.5:1 contrast, and sentence case everywhere else.

## D-10 · No dimming on bypass (Day 2)
- **Rule:** Practical UI says to avoid disabled-looking states.
- **Decision:** the knob and readout stay at full contrast when bypassed. Bypass is shown by the switch, its label, the LED and the skyline together, never by color alone.

## D-11 · Plugin window 768×416 (Day 5)
- **Decision:** the plugin window is 768 × 416.
- **Evidence:** it matches the mockup's proportions.
- **Corrected Day 6:** this entry first claimed a 12-column grid; the built layout uses symmetric 8 pt zones (D-23).

## D-12 · Zen Kaku Gothic New for Japanese text only (Day 5)
- **Rule:** Practical UI says to use one sans-serif typeface.
- **Decision:** Zen Kaku Gothic New for Japanese text only; Jost for everything else.
- **Evidence:** Jost has no Japanese glyphs.
- **Result:** recorded as Practical UI exception D5.

## D-13 · First Figma pass rejected; rebuilt from the mockup (Day 5)
- **Context:** the first Figma pass was built from the tokens instead of the mockup, and lost the texture and craft.
- **Decision:** rejected it and rebuilt with the mockup as the source of truth.
- **Lesson:** give AI the visual reference, not just the spec.

## D-14 · Skyline as generated pixel art: one generator for Figma and JUCE (Day 6)
- **Context:** the v2 vector skyline read as "AI slop." The feedback asked for something that mimics Tokyo, and "fine if it feels a bit like a Tamagotchi."
- **Decision:** a deterministic 112 × 95 grid at 3 pt per pixel, generated by `tools/skyline/skyline_gen.js`. The same code drew the Figma layers and exports the window order for JUCE.
- **Evidence:** PRD §9 requires an integer pixel grid, and a single source means Figma and the plugin can't drift. The landmarks (Tokyo Tower, the Skytree, the Shinjuku government twin towers, the Park Tower peaks) are readable at Tamagotchi scale.
- **Result:** the user's feedback: "the pixel skyline looks genuinely good."

## D-15 · Make the skyline morph legible (Day 6)
- **Context:** City size 0 and 100 looked almost the same in the first states pass.
- **Decision:**
  - Lit windows scale from 0% to 60% of windows.
  - Unlit windows are darker than the facade (`#9F90D2`), so lit windows read.
  - Saturation rises from 70% to 100%.
- **Result:** 0 / 52 / 105 / 158 / 211 windows lit at 0 / 25 / 50 / 75 / 100.

## D-16 · Keep texture off labels; darken ink/secondary (Day 6)
- **Rule (PRD §9):** texture never crosses labels, the readout or controls, and contrast is checked against the darkest texture patch.
- **Decision:**
  - A feathered keep-out mask around every label and control.
  - `ink/secondary` changes from `#5E5486` to `#4F4577` (approved).
- **Evidence:** measured on the darkest 7 px patch next to each label, with glyphs excluded:
  - ink/primary: at least 6.5:1.
  - The old ink/secondary: 3.8:1, which failed.
  - The new ink/secondary: 4.8:1 on the darkest patch and 7.0:1 on plain cream.

## D-17 · The readout is not dimmed on bypass (Day 6)
- **Decision:** follow D-10. PRD §8 "bypassed (dimmed)" is corrected to "unchanged."

## D-18 · Bake static text into the images (approved, Day 6)
- **Decision:** every label, including the Japanese, is baked into the @2x images. Only the readout value is live.
- **Trade-off:** changing a label means re-exporting an image.
- **Benefits:** a smaller binary, no Japanese font in the plugin, and pixel-identical to Figma.

## D-19 · v4 layout on the Practical UI spacing scale (Day 6)
- **Context:** the user's v3 feedback: the knob sat too close to the screen, the switch spacing was off, and "nothing feels properly centered."
- **Cause:** the knob's tick marks overlapped the bezel's bounds by 9 pt. The groups were aligned to nothing in particular, and the switch labels had two different left edges.
- **Decision:**
  - 32 margins and 24 between groups. Groups are spaced more widely than anything inside them (8 and 12).
  - The knob group and switch share the screen's centre line at y 244.
  - The header is centred on y 28, with screws at an 8 inset.
  - The switch labels share one left edge, and the LED sits inline after FX ON.

## D-20 · Header brand line (Day 6)
- **Decision:** ビッグシティ is replaced with サンシャワー・オーディオ (sunshower audio) plus a hand-drawn marker doodle of a sun peeking behind a rain cloud.
- **Also removed:** the palm tree and the jack.
- **Also changed:**
  - Each screw is turned to a different angle, with screwdriver slip marks on two, so it looks like it's been opened and refitted.
  - 35% fewer panel scratches.

## D-21 · 7-segment readout with burned-in digits (approved, Day 6; replaces D2)
- **Context:** the mockup's burned-in "8"s couldn't show through. In Jost, a ghost "88.8" sits almost entirely under the real digits.
- **Decision:** 3½-digit 7-segment digits drawn as shapes, with a burned-in "188.8" at 13%. The half-digit "1" is needed for 100.0.
- **Benefit:** no font is bundled at all.
- **Cost:** about 30 minutes extra for the segment-rendered edit state.

## D-22 · Control edges ≥3:1 on the aged panel, not just on clean cream (Day 6)
- **Context:** a Practical UI audit measured control boundaries against the darkest aged panel next to each control. stroke/strong (`#7D7590`) passed on clean cream (3.54:1) but dropped to 2.5–2.9:1 on the yellowed panel. The knob rim measured about 2.5:1 and the LCD edge about 1.9:1.
- **Decision:**
  - stroke/strong changes to `#676080`.
  - The knob rim changes to `#706248`.
  - The LCD edge changes to `#5F6B49`.
- **Evidence (re-measured, worst case):**
  - Switch track: 3.79:1.
  - Knob rim: 3.14:1.
  - LCD edge: 3.05:1.
  - Readout digits on the LCD: 7.4:1 or better.
- **Also:** the 7-segment digits are thicker (4.5 pt segments, up from 3 pt) after feedback that they were too faint.

## D-23 · 12-column grid exception (Day 6)
- **Rule:** align the layout to a 12-column grid.
- **Finding:** no 12-column grid with spacing on the 8 pt scale can hold the three fixed-size objects. Those are the knob (176, set by touch-target size and the tick ring), the bezel (352, set by the 112-px skyline at 3 pt per pixel) and the switch (128), with equal gaps between them. D-11 previously claimed a 12-column grid; that was not true of the built layout.
- **Decision:** a documented exception. The layout uses symmetric 8 pt zones instead: 32 | 176 | 24 | 352 | 24 | 128 | 32, with one shared centre line. This keeps what the rule is for (consistent alignment and predictable spacing) in a fixed-size instrument panel.

## D-24 · Knob accessibility deferred to Saturday cleanup (Day 7)
- **Context:** the knob is a custom component, not a `juce::Slider`, because Slider can't do Shift fine-drag or a fine scroll wheel, and enabling its Option-click reset also makes a plain double-click reset (which tomorrow's text entry needs). A custom component loses the VoiceOver support Slider gets for free.
- **Decision:** add an accessibility handler to the knob as a Saturday cleanup task, about 45 minutes. It exposes the value, the 0–100 range and the title "City size".
- **If Saturday runs out of time:** it moves to the v1.1 backlog.

## D-25 · Knob motion in sync with the display, with a vector pointer (Day 7–8)
- **Context:** the knob felt "2017-ish". It repainted from a 30 Hz timer, which on a 120 Hz ProMotion display lands every 3–5 refreshes, so the rotor stepped unevenly and trailed a drag by up to about 40 ms. The pointer was part of the rotor bitmap, so its edges softened differently at each angle and shimmered.
- **Decision:**
  - The knob alone repaints in sync with the display (`juce::VBlankAttachment`), and only when its angle moves, **at most 60 Hz** (Day 10). Everything else stays on the 30 fps timer.
  - The pointer is a vector path (4 × 20, radius 2, `ink/primary`, 1 pt `#FFFDF6` 25% highlight, 92% opacity). `knob_rotor.png` is re-exported without it.
  - Visual-only easing: drags and swipes follow 1:1; notches, Option-click and host jumps of more than 1.0 per frame glide over 80 ms with a cubic ease-out. The parameter never eases.
  - The panel crop and `knob_base` are cached as one image, so each frame draws one image, the rotor, the pointer and the overlay.
- **Why the knob tells the motion where a change came from:** a pure "more than 1.0 per frame" rule misfires both ways. A 1.0 wheel notch would never glide, and a fast drag moves more than 1.0 per frame, so it would lag. So the knob's own drags and swipes always follow, its notches and resets always glide, and only host changes are judged by size.
- **Cost:** a fast host automation sweep (more than 1.0 per frame, e.g. 0–100 in under about 0.8 s at 120 Hz) glides and trails slightly while it moves.
- **Update (Day 10): at most 60 Hz.** The CPU benchmark (30 instances, editor open, City size sweeping) measured the knob's repaints at 14.4% of a core on a 120 Hz display, almost all of it macOS redrawing and compositing each frame. The knob still reads the value on every display refresh, but repaints only every other frame on a 120 Hz display (a minimum gap of 1/80 s, so 60 Hz displays still draw every frame). A skipped repaint is drawn on the next allowed frame. Measured: 63.8–65.4% → 60.7% of a core with the editor open while sweeping.

## D-26 · The skyline follows the knob on screen (Day 8)
- **Context:** the PRD says the morph is "driven by the smoothed value" with "ease-in-out", but gives no duration. The audio's 30 ms smoothing is about one frame at 30 fps, so it adds nothing visible.
- **Decision (approved):** the skyline reads the knob's on-screen value. It glides with the knob on jumps and tracks 1:1 otherwise, so the knob and skyline move as one unit (PRD §9). No separate easing.
- **Also approved:**
  - The skyline is clipped to a 14 pt corner radius, matching the bezel art (Practical UI exception D7).
  - Both looks are built now, switching instantly on bypass. The 200 ms fade comes later (cut #1 if time runs short).
- **Glow:** each lit window's glow is an exact blurred square, accumulated per lit count, rather than a blur of the whole layer. The two match where the glows don't overlap.
- **PRD:** §7's "Skyline morph" row now matches this decision (it said "smoothed value; ease-in-out").

## D-27 · The knob is the time of day: midday at 0 to dusk at 100 (Day 10)
- **Decision:** the skyline morph becomes a time of day. Five keyframes (`skyline_tod_000/025/050/075/100.png`) sit at 0, 25, 50, 75 and 100, and the skyline crossfades linearly between the two either side of the knob's on-screen value. The lit windows and their glow stay as they are, so the city lights up as it gets dark.
- **Replaces:**
  - D-15's saturation rule (70% → 100%). `skyline_colour.png` and `skyline_grey.png` are gone.
  - The window desaturation that matched Figma's saturation layer (ui-spec, Day 8). It has no job without that layer.
- **Bypassed:** the greyscale of the current crossfade, with the windows in `#E0E0E0`, at 30%. The grey keyframes are computed once at load from each pixel's luminosity, not shipped as assets.
- **Memory:** the 10 decoded keyframes (15.1 MB) are shared by every open editor and freed when the last one closes, so closed plugins hold no UI images. UI images: 0 / 25.4 / 29.6 MB with 0 / 1 / 3 editors open, calculated from the image sizes; the keyframes' 15,052,800 bytes are measured in the tests.
- **Checked:** the keyframes redraw the middle of the skyline (the pink twin towers became lower buildings and a spire), but all 353 windows in `skyline-windows.json` still land on an unlit-window pixel in every keyframe, so the window order didn't change.
- **Moved out:** water shimmer moves to BC-02 (PRD §5 "Out of v1").
- **PRD:** §7's "Skyline morph" row now says time of day; §12 cut #2 (two-frame crossfade) no longer applies.

## D-29 · Header doodle redrawn by hand (Day 10)
- **Decision:** Sunshower doodle redrawn by hand after beta feedback that it looked AI-generated; lilac-to-peach sunset sun.
- **Asset:** `assets/panel.png` (1536×832) re-exported from Figma; nothing else on the panel changed.
