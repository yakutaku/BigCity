# BigCity: project rules for Claude Code

BigCity is a one-knob macOS VST3 audio effect built with JUCE (C++).
The "City Size" knob (0–100) is a macro. It drives a high-shelf "air" boost and a stereo micro-pitch-shift at the same time, and there is a bypass switch and a pixel-skyline UI.

**`docs/PRD.md` is the source of truth.** If this file and the PRD disagree, follow the PRD and flag the conflict.

## How to work

- **Keep it simple.** Build the simplest thing that meets the PRD. Don't add abstractions, options or features the PRD doesn't ask for.
- **Stay in scope.** Don't touch files or functions the current task doesn't need.
- **Ask first** before you:
  - add a dependency
  - change a parameter ID
  - change build or signing settings
  - delete files
  - do anything irreversible
- **Don't invent audio values.** Use the sound spec table in the PRD. If you need a value that isn't there, stop and ask.
- **Say when you're unsure.** If you're not confident about an approach or an API, say so before writing code.
- **Commit small and often.** Make one commit per working step, and never force-push.
- **Check your work.** At the end of every task, build Release, run pluginval, and report the result in a short summary.

## Code standards (Clean Code, adapted for real-time C++)

We follow Robert C. Martin's *Clean Code*. **In any conflict, real-time audio performance, quality and safety come first.**

- **Names**
  - Names say what a thing is or does, e.g. `airShelfGainDb` rather than `g2`.
  - No abbreviations beyond standard DSP terms (dB, Hz, ms, LFO).
  - Units go in the name: `delayMs`, `cornerHz`.
- **Functions**
  - Small, each doing one thing, at one level of abstraction.
  - Few arguments; group related values in a struct.
- **No magic numbers.** Every audio constant is a named `constexpr` in `Source/SoundSpec.h`, with a comment linking it to its row in the PRD sound spec table.
- **Comments** explain *why*, never *what*. The code should explain the what.
- **Structure**
  - One class per responsibility: `AirShelf`, `MicroShifter`, `CityMacro` (maps the knob to sub-parameters), `AutoGain`, `PluginProcessor`, and one component per UI element.
  - The processor wires the parts together; it doesn't do their math.
- **No duplication.** If the same logic appears twice, extract it.
- **Error handling**
  - Handle invalid states at the boundary (text entry, state restore).
  - **No exceptions and no allocation on the audio thread.**
- **Tests**
  - Unit-test the pure logic, without a host: `CityMacro` mapping, null at 0, bypass crossfade, text-entry parsing and clamping.
  - Tests run as a CMake target.
- **Leave the code cleaner than you found it,** but only inside files the current task touches.

**Where Clean Code bends:**
- **Performance.** Inside `processBlock`, and inside anything it calls, prefer plain loops over abstractions that allocate or dispatch virtually per sample.
- **Language.** Clean Code's Java-centric advice (e.g. deep class hierarchies) doesn't apply. Prefer simple value types and composition.

## Design standards (Practical UI, 2nd edition)

All UI decisions follow Adham Dannaway's *Practical UI*. **The rules are in `docs/practical-ui.md`. Read it before any UI work and check every UI change against it.**

- **Reference file.** The user's Practical UI Design System Starter Kit is in Figma (file key `4hWRpgXXP0AEle5x7GQK1i`). It supplies the token structure below.

**Token structure from the starter kit** (values adapted to BigCity in `Source/Tokens.h`):
- **Spacing:** 8 pt grid (8, 16, 24, 32, 48, 80, 96). Use 4 and 12 only for tight spacing inside a component, as the starter kit does.
- **Type scale:** 40/48, 32/40, 20/28, 16/24, 14/20 (size/line height), in **Jost**.
  - Two weights only: Regular 400 and Bold (Semi Bold 600).
  - Sentence case everywhere, except the plugin's control and header labels. Those use the uppercase style (Semi Bold 14/24, +2 letter spacing), per Practical UI decision D1.
- **Text colors:** two emphasis levels only, strong and weak. Both must meet 4.5:1 on their background.
- **Strokes:** weak for decoration; strong for control edges, which must meet 3:1.
- **Corner radius:** 8 / 16 / 32.
- **Shadows:**
  - raised: 0 4 8 −2 at 4%, plus 0 2 4 −2 at 8%
  - overlay: 0 20 24 −4 at 8%, plus 0 8 8 −4 at 4%

- **Interactive states:** hover and press use transparent overlays, never swapped solid colors.

Never invent a spacing, size or color outside these scales. If a layout seems to need one, stop and ask.

## Stack

- **JUCE 8.x:** the latest 8.0.x release tag, as a git submodule at `/JUCE`. Build with CMake only, not Projucer.
- **C++20**
- **Formats:**
  - VST3 is the only format that ships.
  - Standalone is for development testing only.
  - No AU and no Windows in v1.
- **Architectures:** universal binary, `CMAKE_OSX_ARCHITECTURES="arm64;x86_64"`
- **Minimum macOS:** 11.0
- **Plugin identity** (final; never change after the first beta):
  - Company: "sunshower audio" (all lowercase, deliberately)
  - Manufacturer code: `Snsh`
  - Plugin code: `Bgct`
  - Bundle ID: `com.sunshoweraudio.bigcity`

### Commands

```bash
cmake -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build --config Release
/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 5 --validate "$HOME/Library/Audio/Plug-Ins/VST3/BigCity.vst3"
```

Use `COPY_PLUGIN_AFTER_BUILD TRUE` so every build installs itself to `~/Library/Audio/Plug-Ins/VST3`.

## Real-time audio rules (non-negotiable)

**Inside `processBlock`:**
- No memory allocation, locks, file or network I/O, logging, or UI calls.
- Use `juce::ScopedNoDenormals`.

**Buffers and formats:**
- Handle any buffer size and sample rates from 44.1 to 192 kHz.
- `prepareToPlay` resets all audio state.
- Support stereo in/out and mono in/out.

**Parameters:**
- All parameters live in one `AudioProcessorValueTreeState`.
- Smooth `citySize` with `juce::SmoothedValue`, about 30 ms.

**Output behavior:**
- **Latency:** report 0. The micro-shift delay is part of the effect, not latency.
- **At 0:** the output must match bypass (null test at −100 dB or better).
- **Bypass:** crossfade over about 20 ms with no clicks. Expose it through `getBypassParameter()` so the host and the UI switch share one parameter.

## Parameters

**Parameter IDs are frozen after the first beta.** Renaming one breaks saved Ableton sessions.

| ID | Type | Range | Default | Notes |
|---|---|---|---|---|
| `citySize` | float | 0–100, step 0.1 | 50 | Label "City size", unit "%" |
| `bypass` | bool | on/off | off | Returned by `getBypassParameter()` |

State save and restore go through the APVTS.

## UI rules

**Rendering:**
- Native JUCE drawing, no WebView.
- The window is a fixed size, set from Figma on Day 6.
- Use @2x raster assets, and draw pixel art with nearest-neighbour scaling.

**Performance:**
- Repaint only when a value changes, with a timer of at most 30 fps.
- Cache static layers.

**Design tokens:**
- All colors, fonts and spacing live in `Source/Tokens.h`, mirrored from the Figma variables.
- Never hard-code a hex value outside that file.

**Figma:**
- Use the Figma MCP (`get_variable_defs`, `get_screenshot`, `get_design_context`) for reference only.
- Its code output is web code. Adapt it to JUCE; never paste it in.

## Git

- Never commit `build/`, `.DS_Store`, or anything from `~/Library`.
- Commit messages are short and imperative, e.g. `Add citySize parameter and smoothing`.

## Definition of done (every task)

1. The Release universal build succeeds with no new warnings.
2. pluginval passes at strictness 5.
3. It loads and plays in Ableton Live.
4. Sweeping the knob or toggling bypass while audio plays makes no clicks or zipper noise.
5. Changes are committed, with a two-line summary of what changed and what's next.
