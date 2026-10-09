# Next steps (Day 8, 2026-10-09)

- **Done:** knob Fix 1 and Fix 2 (D-25, merged to main); the skyline with its morph, glow and bypassed look (D-26); the bypass switch with an instant skyline switch. The skyline and switch are on `wip/day8-skyline`, not merged yet.
- **Waiting on the user:** the Ableton knob checks (hover, drag and swipe, jumps), and the Figma sign-in so the window glow can be compared with Figma.
- **Beta 0.9.0 (decided Day 8):** ships tonight without text entry, with the instant bypass. After the user's Ableton QA passes, run `tools/package.sh --notarize`; without the flag it builds, signs and packages but stops before notarizing.
- **Next:** readout text entry (the first beta update), then the 200 ms bypass fade on Monday if there is time, otherwise cut #1 stands.
- **Saturday cleanup:** knob and switch accessibility (D-24). If the knurl still shimmers when turning, consider drawing it as vectors too.
