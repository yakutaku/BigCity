# Next steps (paused Day 7, 2026-10-08)

- **Done:** Fix 1. The knob's hover, press, cursor, clicks and scrolling apply only to the cap, a circle of radius 56 pt around the knob centre. It's tested, and ui-spec is updated (commit 7402dba).
- **Done:** the Fix 2 measurements. The knob repaints at most 30 Hz, out of sync with the 120 Hz display. The rotor already uses high-quality resampling (kCGInterpolationHigh). The pointer comes from Figma `I5:101;5:51`: 4 × 20 pt, radius 2, `#3B3160` at 92%, with an inset `0 1 0 0` highlight in `#FFFDF6` at 25%.
- **Half-done:** Fix 2 (a: display-synced repaint, b: vector pointer, c: glide only when a jump is over 1.0 per frame) is approved, but none of its code is written yet. The new tokens, CLAUDE.md, ui-spec and decision D-25 are also still to do.
- **Blocked on:** a pointer-less `assets/knob_rotor.png` (384 × 384, transparent). The Figma frame is `25:7862`. The MCP download flattens it onto an opaque grey background, so don't use that file.
- **First step to resume:** export `knob_rotor` from Figma by hand into `assets/knob_rotor.png`, check it's transparent and has no pointer, then build Fix 2 a → b → c as planned. After that: the skyline plan, the bypass switch with its 200 ms fade, then readout text entry.
