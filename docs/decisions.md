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

## D-11 · Plugin window 768×416 on a 12-column grid (Day 5)
- **Decision:** the plugin window is 768 × 416, laid out on a 12-column grid.
- **Evidence:** it matches the mockup's proportions.

## D-12 · Zen Kaku Gothic New for Japanese text only (Day 5)
- **Rule:** Practical UI says to use one sans-serif typeface.
- **Decision:** Zen Kaku Gothic New for Japanese text only; Jost for everything else.
- **Evidence:** Jost has no Japanese glyphs.
- **Result:** recorded as Practical UI exception D5.

## D-13 · First Figma pass rejected; rebuilt from the mockup (Day 5)
- **Context:** the first Figma pass was built from the tokens instead of the mockup, and lost the texture and craft.
- **Decision:** rejected it and rebuilt with the mockup as the source of truth.
- **Lesson:** give AI the visual reference, not just the spec.
