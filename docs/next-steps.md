# Next steps (Day 10, 2026-10-12)

## Where we are

- **On main and pushed:**
  - readout text entry (passed in Live)
  - the time-of-day skyline (D-27)
  - the 200 ms bypass fade
  - the CPU benchmark tool (`Tests/CpuBenchmark.cpp`)
  - the knob's 60 Hz repaint cap (D-25)
  - the v1.1 review (`docs/v1.1-review.md`)
- **The last checked build** (`e1403a1`; `be03889` only adds docs): the Release universal build succeeds, the tests pass, and pluginval passes at strictness 5. The dev build in `~/Library/Audio/Plug-Ins/VST3` matches main.
- **Version** is still 0.9.0 (`project()` in `CMakeLists.txt`).

## P1-2 and P1-3: not run yet

There are no results yet. Both are written up in `docs/v1.1-review.md`, but no test has been written or run.

- **P1-2:** state-restore tests, covering a wrong tag, truncated data, a missing parameter, an out-of-range value and a `nan`. We still don't know whether a `nan` can reach the audio. About 45 min.
- **P1-3:** the null and bypass tests at 88.2, 176.4 and 192 kHz, with block sizes 1, 31, 33, 4097 and varying. About 30 min.

These are the only review items that could reveal a launch bug. Decide first thing whether to run them before the final build.

## Launch checklist (Tue 2026-10-13, PRD §12 day 11)

1. **Decide on P1-2 and P1-3** (above). If they run and find a bug, fix it before going on.
2. **Check in Live** on the dev build. These haven't been checked in Live yet:
   - the time-of-day skyline from 0 to 100
   - the 200 ms bypass fade, including toggling it quickly
   - sweeping the knob and toggling bypass with audio playing, listening for clicks or zipper noise (definition of done #4)
3. **Live CPU check (P1-1).** Load 30 instances at 48 kHz / 128 samples and automate City size. Compare Live's CPU meter with one editor open and with all editors closed. If the open editor costs much more than the benchmark's ~25–30% of a core, it's a v1.1 item, not a launch blocker.
4. **Ableton matrix (PRD §11):**
   - 44.1 / 48 / 96 kHz
   - buffers of 32 / 128 / 1024
   - save and reload a session
   - automation
   - freeze and flatten
   - offline export
   - mono and stereo tracks
   - 30 instances
5. **Beta results.** The results table in `docs/beta-protocol.md` is still empty. Record what the producers reported, and fix anything that blocks launch.
6. **Set the launch version.** Choose the version (e.g. 1.0.0). It's a build setting, so I'll ask before changing `project()`.
7. **Final build:** build Release, run the tests and pluginval at strictness 5, then commit.
8. **Package:** run `tools/package.sh --notarize`.
9. **Clean install.**
   - Before testing, remove the dev build from `~/Library/Audio/Plug-Ins/VST3`, so Live doesn't load two copies.
   - The 0.9.0 beta that was moved out of `/Library/Audio/Plug-Ins/VST3` must stay out.
   - Install from the `.pkg` on the M4, the friend's M2, and under Rosetta (PRD §4).
10. **Release (PRD §10):** zip the `.pkg` with the quick-start PDF, then publish the Gumroad listing (pay what you want, $0 minimum). I don't know the status of the PDF or the listing; neither is in the repo.

## Open items

- **Live CPU check (P1-1):** step 3 above. This decides whether UI CPU is a v1.1 item.
- **Leftover branches.** Nothing has been deleted; deleting any of these needs your OK.
  - `backup-bigcity-oops`: local only
  - `tuning`: 11 commits ahead of `origin/tuning`, unpushed
  - `wip/day7-knob-motion` and `wip/day8-skyline`: merged; also on origin
  - `wip/knob-60hz-cap`: merged; local only
  - `wip/shared-tick`: the unmerged shared-tick experiment; local only (no clear win, see D-25)
- **v1.1 list:** `docs/v1.1-review.md`. It has no P0 items.
  - P1-1: UI CPU
  - P1-2 and P1-3: tests, unless they run before launch
  - P1-4: knob and switch accessibility (D-24)
  - P2 and P3: duplication, idle CPU and housekeeping, including the stale knob line in CLAUDE.md (P3-7)
