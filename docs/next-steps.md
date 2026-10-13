# Next steps (Day 11, 2026-10-13)

## Where we are

**BigCity 1.0.0 is released and live on Gumroad.**

- **Tag:** `v1.0.0` on main at `89471d0`, pushed.
- **Installer:** `dist/BigCity-1.0.0.pkg` (not in the repo)
  - 7,570,367 bytes
  - SHA-256 `41c569384274b41b82b745d014ef078cafec1276afca040fcbd9dd1c1e0b9d3c`
- **Signing and notarization:**
  - The installer is signed with Developer ID Installer and the plugin with Developer ID Application (hardened runtime).
  - Notarized on the first submission (`2253f6e9-ac95-456a-8695-1ae48c74656a`) and stapled.
  - `spctl` accepts it as "Notarized Developer ID".
- **Final build:**
  - The clean Release universal build succeeds with no new warnings.
  - All tests pass, and pluginval passes at strictness 5.
- **Sound:** voicing A from the beta (D-30, PRD §6 "Knob response"). It passed the Ableton check.

## Launch checklist (done 2026-10-13)

1. [x] P1-2 and P1-3 ran before the final build, and the `nan` restore bug they found is fixed (`docs/v1.1-review.md`).
2. [x] Checked in Live on the dev build.
3. [x] Live CPU check (P1-1).
4. [x] Ableton matrix (PRD §11).
5. [x] Beta results. The feedback that changed the sound is recorded in PRD §6 "Voicing change" and D-30. The results table in `docs/beta-protocol.md` is still blank.
6. [x] Version set to 1.0.0 (`project()` in `CMakeLists.txt`, the only place it lives).
7. [x] Final build, tests and pluginval.
8. [x] Packaged, notarized and stapled with `tools/package.sh --notarize`.
9. [x] Clean install from the `.pkg`.
10. [x] Released on Gumroad.

## Open items

- **Branches.** The merged local-only branches were deleted after the release: `voicing/subtle`, `shimmer-decision`, `docs/practical-ui`, `test/p1-state-and-rates` and `wip/knob-60hz-cap`. These remain, and deleting any of them needs your OK.
  - Not merged: `backup-bigcity-oops`, `fix/shimmer-memory`, `wip/shared-tick`. All three are on origin.
  - Merged, kept on origin as backups: `tuning`, `wip/day7-knob-motion`, `wip/day8-skyline`, `wip/water-shimmer`.
- **v1.1:** the PRD §5 v1.1 backlog and `docs/v1.1-review.md`. Neither has P0 items. They include:
  - UI CPU and the JUCE renderer's memory (P1-1, audit A2)
  - knob and switch accessibility (P1-4, D-24)
  - the flat top of the knob response
  - an optional output ceiling
  - recalibrating auto gain on music
