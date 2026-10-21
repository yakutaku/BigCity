# Case study evidence plan (S tier)

**The bar:** every claim in the case study is backed by an artifact or a number. The user writes the copy; this file tracks the evidence.

## Evidence checklist

| # | Evidence | Source | When | Status |
|---|---|---|---|---|
| 1 | Decision log (10+ hard calls with reasoning) | `docs/decisions.md` | Ongoing | 🟡 Seeded (D-01 to D-21) |
| 2 | Time to dial in (target under 10 s) | Beta test 1 | 10/10–11 | ⏳ |
| 3 | Blind A/B ratings at 25/50/75/100 | Beta test 2 | 10/10–11 | ⏳ |
| 4 | Tester quotes, with permission to quote | Beta test 3 | 10/10–11 | ✅ Permission granted 10/17 to quote and credit by artist name. Recording clips not needed. |
| 5 | CPU benchmark: 30 instances on the M2, % CPU and buffer size recorded | Day 9 | 10/9 | ⏳ |
| 6 | Loudness match: auto gain old vs new table (pink noise vs music) | D-08 | 10/5 | ✅ |
| 7 | Null test: 0 is bit-identical to bypass | Unit tests | Done | ✅ |
| 8 | Mapping-curve chart: City size → air dB, detune, shimmer dB, compensation | SoundSpec.h values | Day 10 | ⏳ |
| 9 | State inventory sheet: every knob, switch and skyline state | Figma | Day 6 | ✅ Day 6 |
| 10 | Mockup → final, annotated with Practical UI rules and decisions D-09/D-10 | Figma | Day 6 | ✅ Day 6 |
| 11 | Contrast table: every token pair with its ratio (WCAG AA) | Figma, Coolors | Day 5 | ✅ Day 6 |
| 12 | 15 s video: knob sweep, skyline morph, bypass transition, with audio | Screen recording | Day 10 | ⏳ |
| 13 | AI workflow artifact: CLAUDE.md excerpt, a plan you reviewed and changed, a moment you overrode the AI | Repo and this log | Day 10 | ⏳ |
| 14 | Downloads and paid ratio | Gumroad | Post-launch | Later |
| 15 | Instruments trace of CPU at 30 instances (with the CPU benchmark) | Instruments, alongside #5 | 10/9 | ⏳ |
| 16 | pluginval strictness 5 pass on every build, as a quality gate | pluginval runs at the end of every task (CLAUDE.md "Definition of done") | Ongoing | 🟡 Passing every build so far |

## AI-direction moments worth showing (examples so far)
- Centered the micro-shift window on the pre-delay after reviewing the AI's plan. Its version would have pushed the average delay outside the sourced range.
- Kept Q 0.707 when the AI was about to revert to 0.5.
- Kept the rescaled curves "as heard" instead of locking only the maximums.
- Rejected uninstalled tooling (clangd LSP) and auto mode while the project was new; enabled auto mode once the steps became routine.
- Required "plan first, wait for OK" on every substantial task.
- Rejected the first Figma pass, which the AI built from the tokens instead of the mockup and which lost the texture and craft. It was rebuilt with the mockup as the source of truth. Lesson: give AI the visual reference, not just the spec (D-13).

## Daily habit (5–10 minutes)
At the end of each day, add any new decision to `decisions.md` and update this table.
