# BigCity PRD v1.1

**Status:** Build in progress · **Updated:** 2026-10-02 · **Launch target:** Tue 2026-10-13

This version replaces v1.0. The earlier timelines are void.

---

## 1. Summary

BigCity is a lightweight one-knob macOS VST3 effect for Ableton producers.

- **The knob.** "City Size" (0–100) is a macro. It adds "air" (a high-shelf boost) and a stereo micro-pitch-shift together, along a tuned curve, so that every knob position sounds good.
- **The interface.** A bypass switch and a pixel city skyline that brightens as the knob turns.
- **The feel.** It should feel like a piece of 80s Japanese gear found in a Shimokitazawa vintage shop in 2026.

## 2. Problem

- **Plugins are too complex and too heavy.** Many are expensive and CPU-heavy, and each one asks producers to learn a new interface to reach one specific sound.
- **Bedroom producers feel it most.** They're often on older Macs, and dense plugin chains strain those machines.
- **City pop has no easy starting point.** Producers outside the genre's traditions have no simple way to add its sheen.

BigCity does one job, costs little CPU, and needs no learning.

## 3. Audience

- **Primary:** in-the-box Ableton producers on macOS making vaporwave, city pop, future funk or digicore.
- **Secondary:** EDM and pop producers looking for new textures.

## 4. Success criteria

| Criterion | How we measure it |
|---|---|
| Installs and opens without issue | Clean install from the signed installer on the M4, the friend's M2, and under Rosetta |
| Stable | pluginval passes at strictness 5; no crashes during beta |
| Light on CPU | 30 instances on the M2 at 48 kHz / 128 samples with no dropouts. The measured number is recorded for the case study. |
| Transparent at 0 | Nulls against bypass at −100 dB or better |
| Fast to dial in | Beta producers reach a sound they like in under 10 seconds (observed) |
| Sounds good at every position | Blind A/B test with 2–3 producers at 25 / 50 / 75 / 100 |

## 5. Scope

**In v1**
- One macro knob and a bypass switch
- Pixel skyline that responds to the knob
- VST3 only, macOS 12+, universal binary
- Signed and notarized installer
- Quick-start PDF
- Gumroad listing

**Out of v1**
- Presets
- AU, Windows, AAX
- Automation indicator (Ableton already shows automation)
- Arrow-key control (Ableton doesn't reliably pass keys to plugins; scroll wheel replaces it)
- Resizable window
- Buildings growing or animating

**v1.1 backlog**
- Matched (analog-like) shelf design for the air shelf, so its response no longer depends on sample rate (see the §6 shelf check finding)

## 6. Sound spec

Every value is tagged with its source. **We don't guess.**

- **Sourced:** backed by published practitioner or manufacturer data.
- **Measure:** set by our own measurement.
- **Test:** a starting point, confirmed or changed by blind A/B in beta.
- **Tuned by ear 10/2:** set by listening in the developer tuning build on 2026-10-02. Still open to change by blind A/B in beta.

| Element | Value | Status | Basis |
|---|---|---|---|
| Signal flow | Dry at unity gain, plus an air shelf and a micro-shift wet signal. The macro scales both, and auto gain levels the result. | — | Design decision |
| At 0 | Fully transparent (identical to bypass) | — | Design decision |
| Default | 50 | — | Industry convention (NI, Kilohearts) |
| **Air:** filter | RBJ high-shelf biquad, one per channel | Decided v1 (user research) | RBJ Audio EQ Cookbook |
| **Air:** corner frequency | 10 kHz | Tuned by ear 10/2 | The low end of the v1.0 PRD's 10–12 kHz range |
| **Air:** Q | Fixed 0.707 | Tuned by ear 10/2 (A/B listen: Q 0.707 had more top end) | Broad "air" shelves are the reference sound (Mäag Air Band) |
| **Air:** gain curve | 0 dB at 0 → +7.8 dB at 100; `gainDb = 7.8 × citySize / 100` | Tuned by ear 10/2 | dB is perceptually close to linear |
| **Micro-shift:** detune | Left down, right up; ±5.5 cents at 0 → ±7.7 at 50 → ±11 at 100 | Tuned by ear 10/2 | Practitioner range: H3000 micropitch recipes use ±7 cents (Splice preset 519), 8–10 cents commonly, 4–12 cents overall |
| **Micro-shift:** pre-delay | Left 11 ms / right 17 ms, fixed | Sourced range, values Test | 6–20 ms common, offset left and right |
| **Micro-shift:** wet level | Off at 0 → −9 dB at 50 → −3 dB at 100 | Tuned by ear 10/2 | "Turn it up until you hear it, then back off" |
| **Micro-shift:** algorithm | Dual-tap crossfading delay-line shifter (H910-style) | — | Lowest CPU; small shifts produce minimal artifacts |
| **Micro-shift:** detune curve | Linear between points: ±5.5 cents at 0 → ±7.7 at 50 → ±11 at 100 | Tuned by ear 10/2 | The v1 curve's shape (5 : 7 : 10) scaled to the tuned maximum. Inaudible at 0 because the wet signal is off. |
| **Micro-shift:** wet level curve | 0–50: linear in amplitude from off to −9 dB. 50–100: linear in dB from −9 to −3 dB. | Tuned by ear 10/2 | The v1 curve's shape (6 dB from 50 to 100) moved to the tuned maximum. "Off" has no dB value; linear in dB above 50 matches the air curve. |
| **Micro-shift:** window | 20 ms, centered on the pre-delay (left sweeps 1–21 ms, right 7–27 ms) before modulation. Two taps half a cycle apart with sin² crossfades; 4-point cubic Hermite interpolation. | Test | About one crossfade cycle per 3.1 s at ±11 cents |
| **Micro-shift:** modulation | Sine LFO at 0.72 Hz sweeps each voice's delay over 1.65 ms in total, upward from the pre-delay; left and right in opposite phase | Tuned by ear 10/2 | Juno-60 chorus: a slow LFO with the two sides in opposite phase. Upward only because the left window already reaches down to 1 ms; the average delay sits 0.83 ms above the pre-delay. |
| **Wet:** high-pass | 250 Hz, 12 dB/oct Butterworth | Tuned by ear 10/2 | MicroShift's Focus control: keeps the shifted signal out of the low end |
| **Wet:** low-pass | 7 kHz, 12 dB/oct Butterworth | Tuned by ear 10/2 | MicroShift's Focus control; also smooths the drive's added harmonics |
| **Wet:** drive | 3 dB into a level-compensated tanh, `tanh(g·x) / g`, between the high-pass and the low-pass; no oversampling | Tuned by ear 10/2 | Gentle saturation, as in hardware shifters' wet path. Quiet signals pass at unity; only peaks round off. |
| Signal order | Output = auto gain × air shelf(dry + wet level × low-pass(drive(high-pass(micro-shift(dry))))) | Test | The air reaches the wet signal too, with one shelf per channel. The wet level comes last in the wet path, so at 0 the wet signal is exactly silent. |
| Auto gain | Fixed compensation curve: one value per 10 of City Size, linear in dB between points, following the smoothed City Size. Exactly 0 dB at 0. Values in `SoundSpec.h`. | **Measure** | Pink noise at −18 dBFS RMS through the plugin at 48 kHz; K-weighted loudness (ITU-R BS.1770) matched to bypass within 0.5 dB |
| Smoothing | 30 ms on `citySize` | — | Prevents zipper noise |
| Bypass crossfade | 20 ms | — | Click-free |

**Shelf response check (Day 3):** measure the air shelf's frequency response at 44.1, 48 and 96 kHz. Flag it if the responses differ by more than 0.5 dB anywhere below 16 kHz.

**Finding (Day 3, accepted for v1; re-measured 10/2 with the tuned 10 kHz / +7.8 dB shelf):** at City Size 100, the 44.1 and 96 kHz responses differ by up to 0.77 dB at Q 0.707, and only above 11.9 kHz. (At Q 0.5 it is 0.94 dB above 13.1 kHz.) On Day 3, at 12 kHz / +6 dB, it was 0.84 dB above 13.7 kHz. The cause is the standard digital design's frequency squeeze near Nyquist. A matched shelf design is on the v1.1 backlog (§5).

## 7. Interaction spec

| Input | Behavior |
|---|---|
| Vertical drag | Changes the value; the full range takes about 200 px of travel |
| Shift + drag | Fine control, 0.1 steps |
| Scroll wheel / trackpad | Changes the value; Shift for fine |
| Double-click | Opens an inline text field (0–100); Enter commits and clamps; Esc cancels |
| Option-click | Resets to 50 |
| Hover | White overlay at 10%; vertical double-arrow cursor on the knob, pointer on the switch |
| Press / drag | Ink overlay at 8% |
| Bypass on | Skyline desaturates to greyscale at 30% opacity over 200 ms with ease-in-out. The knob and readout stay fully interactive and at full contrast (Practical UI decision D4). |
| Host automation | Knob and skyline follow the automation in real time |
| Skyline morph | Driven by the smoothed value; windows light up progressively and saturation rises; ease-in-out |

## 8. State inventory

These states are designed in Figma and implemented in code:

- **Knob:** idle, hover, drag, fine drag, text entry, reset
- **Switch:** FX on, bypassed, hover
- **Whole plugin:** normal, bypassed
- **Skyline:** frames at 0, 25, 50, 75 and 100, plus the bypassed look
- **Readout:** normal, editing, bypassed (dimmed)

## 9. Visual spec

**Palette.** Summer light pastels for surfaces, the skyline, stripes and the knob arc. Text uses deep inks from the same hue family. Contrast is measured on the cream background `#EFE7D4`.

| Token | Hex | Contrast | Use |
|---|---|---|---|
| ink/primary | `#3B3160` | 9.5:1 | Labels, knob pointer, active switch label |
| ink/secondary | `#5E5486` | 5.5:1 | Inactive labels, model line. **Not on pink or lavender surfaces.** |
| ink/teal | `#1F6F68` | 4.8:1 | Accent text |
| ink/rose | `#A63D5A` | 5.0:1 | Accent text |
| pastel/pink | `#F4A6B7` | decorative | Primary ink on it: 6.1:1 |
| pastel/peach | `#F9C8A0` | decorative | 7.7:1 |
| pastel/butter | `#F7E3A1` | decorative | 9.2:1 |
| pastel/mint | `#A8DCCB` | decorative | 7.7:1 |
| pastel/sky | `#A9D3EE` | decorative | 7.4:1 |
| pastel/lavender | `#C3B6E8` | decorative | 6.2:1 |

**Contrast rules:**
- Practical UI minimums: 4.5:1 for text, 3:1 for UI components.
- The knob arc is decorative; the pointer and the readout carry the value.
- The switch track needs an edge of at least 3:1.
- Verify every pair in Coolors before the design is frozen.

**Type:**
- Jost (SIL Open Font License) for all labels.
- The readout font's license must allow bundling; DSEG7 (OFL) is the fallback.

**Header:**
- Logo, ビッグシティ, エアー＆レジャー (pending a native speaker's review), MODEL BC-01 (uppercase labels are an approved exception; see Practical UI decision D1).
- Removed: "Made with luv," the 理科準備室 tag, and the empty circle.

**Texture:**
- Scratches, scuffs and paper texture stay.
- They never cross labels, the readout or controls.
- Text contrast is checked against the darkest texture patch.

**Hierarchy:**
- The knob and skyline read as one unit and carry equal weight: the skyline through size, the knob through contrast.
- Their vertical centers are aligned and the gap between them is tight.

**Skyline:**
- Pixel buildings with lit windows and a water reflection.
- Exported from Figma as layers on an integer pixel grid.

## 10. Packaging and release

- A signed and notarized `.pkg` that installs `BigCity.vst3` to `/Library/Audio/Plug-Ins/VST3`.
- Zipped together with a one-page quick-start PDF.
- Gumroad listing: pay-what-you-want, $0 minimum.
- The JUCE Starter license covers revenue under $20k/yr.

## 11. Test plan

| Area | Checks |
|---|---|
| Automated | pluginval strictness 5 on every build |
| Ableton matrix | 44.1 / 48 / 96 kHz; buffers of 32 / 128 / 1024; save and reload a session; automation; freeze and flatten; offline export; mono and stereo tracks; 30 instances |
| Machines | M4 (dev), friend's M2, Intel slice via Ableton under Rosetta |
| Beta | 2–3 producers; blind A/B at 25 / 50 / 75 / 100; time-to-dial-in observed |

## 12. Timeline (4 hrs/day)

| Day | Date | Work | Done when |
|---|---|---|---|
| 1 | Tue 9/29 | Environment setup | ✅ |
| 2 | Wed 9/30 | Docs, scaffold | Pass-through plugin loads and passes pluginval |
| 3 | Thu 10/1 | Sound v1 | |
| 4 | Fri 10/2 | Tuning, auto gain | Sound frozen; test build to one producer |
| 5 | Mon 10/5 | Figma: tokens, layout, knob, switch | |
| 6 | Tue 10/6 | Figma: skyline, states, texture, specs, export | Design frozen |
| 7 | Wed 10/7 | UI build 1 | |
| 8 | Thu 10/8 | UI build 2 | |
| 9 | Fri 10/9 | Sign, notarize, installer, QA | Beta out |
| 10 | Mon 10/12 | Quick-start PDF, Gumroad, case-study visuals | |
| 11 | Tue 10/13 | Beta fixes, final build | **Launch** |

**Cuts if behind, in order:**
1. Instant bypass desaturate instead of the 200 ms fade.
2. Two-frame skyline crossfade.
3. Case-study visuals move to after launch.

## 13. Open items

- [x] Maker name: **sunshower audio** (all lowercase) (`com.sunshoweraudio.bigcity`, manufacturer code `Snsh`)
- [x] Practical UI rules written to `docs/practical-ui.md`. D1 is an approved exception (uppercase labels); D2–D4 follow the book.
- [x] Minimum macOS: **12.0**. Xcode 27 supports 12.0–27.0.
- [ ] Native speaker review of エアー＆レジャー
- [ ] Apple approval → Developer ID Application and Installer certificates (needed by Day 9)