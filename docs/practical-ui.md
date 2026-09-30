# Practical UI rules for BigCity

This file summarizes the principles of Adham Dannaway's *Practical UI* (2nd edition) that BigCity follows, and how each one applies to this plugin.

**Every UI change is checked against this file.** Where BigCity makes an exception, the exception and its reason are written down here.

---

## Fundamentals and usability

| Rule | How it applies to BigCity |
|---|---|
| Every design detail needs a logical reason, not just an aesthetic one | Each visual decision gets a one-line rationale in its Figma annotation. These rationales also feed the case study. |
| Keep related actions close; touch targets at least 48 × 48 pt | The knob hit area is at least 48 pt. The bypass switch hit area is at least 48 pt and includes its labels. The value readout is at least 48 pt tall. |
| Reduce cognitive load; use familiar patterns | Vertical drag, Shift for fine control, double-click to type, Option-click to reset. These are the plugin-industry conventions. |
| Prioritize with the 80/20 rule | One knob and one switch. Nothing else competes for attention. |
| Meet WCAG 2.1 AA | 4.5:1 for text; 3:1 for large text and UI components; state is never shown by color alone. |

## Layout and spacing

| Rule | How it applies to BigCity |
|---|---|
| Group related items with containers, proximity, similarity or alignment | Knob, label and readout form one group. Switch, LED and labels form another. |
| Build hierarchy with size, color, contrast, spacing, position and depth | The knob and skyline share equal weight: the skyline through size, the knob through contrast and depth. The header is secondary. |
| 8 pt spacing grid | 8, 16, 24, 32, 48, 80, 96. 4 and 12 only inside components. |
| Align the layout to a 12-column grid | The plugin window uses a 12-column grid with margins and gutters from the 8 pt scale. |
| Left-align text; don't mix alignments within one component | See decision D3. |

## Typography

| Rule | How it applies to BigCity |
|---|---|
| One sans-serif typeface | Jost for everything, including the value readout (decision D2). The logo is a brand mark and is exempt. |
| Two weights: regular and bold | Jost Regular 400 and Semi Bold 600 |
| Body text at least 18 px with at least 1.5 line height | Applies to the quick-start PDF and the Gumroad page. The plugin has no body text. |
| Line length of 40–80 characters | Quick-start PDF and Gumroad page |
| No pure black on white, no very light grey | Inks are deep indigo tones on cream. The mockup's pale greys are replaced. |

## Color

| Rule | How it applies to BigCity |
|---|---|
| Small text 4.5:1; UI components and large text 3:1 | Verified per token in PRD §9. Coolors check before the design is frozen. |
| Never rely on color alone | Bypass is shown by switch position, the label, the LED and the skyline change together. |
| Red, amber and green only for error, warning and success | No red, amber or green is used for decoration or interactive UI. The FX-on LED is teal. The rose ink is not used for text, so it can't read as an error. Pastel mint appears only in skyline decoration. |
| The brand color marks interactive elements | `ink/primary` (#3B3160) is the interactive color: knob pointer, switch thumb edge, readout text. |
| Hover and press use transparent overlays | Hover: white overlay at 10%. Press: ink overlay at 8%. This replaces the PRD's "+10% brightness." |

## Copywriting

| Rule | How it applies to BigCity |
|---|---|
| Concise; sentences under 20 words | Plugin labels, quick-start PDF, Gumroad page |
| Sentence case, never title case or uppercase | Sentence case for the quick-start PDF, the Gumroad page and the readout. **Exception:** control and header labels on the plugin are uppercase (decision D1). |
| Front-load the important words | Quick-start headings start with the action, e.g. "Turn the knob to add sparkle" |
| Inverted pyramid for longer text | Gumroad description: what it does → why → details |
| Numbers as numerals | "0–100", "30 instances" |
| Descriptive links; never "click here" or "learn more" | Gumroad and portfolio links name their destination |

## Buttons, controls and forms (applied to the plugin's controls)

| Rule | How it applies to BigCity |
|---|---|
| Primary, secondary and tertiary weights; one primary per screen | The knob is the primary control. The bypass switch is secondary. |
| Avoid disabled states | Controls stay fully interactive and legible when bypassed (decision D4) |
| Friction for destructive actions | Not applicable; nothing in the plugin is destructive |
| Field width matches expected input | The text-entry field fits "100.0", 5 characters |
| No placeholder text as a label | The "City size" label stays visible while typing |
| Radio buttons for fewer than 10 options; steppers for small numbers | Not applicable. The knob follows the plugin-industry convention for continuous values. |

---

## Decisions where the book and the mockup disagree

These resolve toward the book unless the user approved an exception. Each exception records its reason.

| # | Conflict | Decision |
|---|---|---|
| D1 | The mockup uses uppercase hardware-style labels ("CITY SIZE", "FX ON") | **Exception, approved by the user: uppercase for plugin control and header labels only** ("CITY SIZE", "FX ON", "BYPASS", "MODEL BC-01"). *Reason:* this follows the plugin-industry convention (Valhalla, Serum) and the hardware reference, and the labels are short enough to read easily. *Guardrails:* use the starter kit's uppercase style (Jost Semi Bold 14/24, +2 letter spacing), keep labels to 1–2 words, and meet 4.5:1 contrast. Everywhere else uses sentence case. |
| D2 | The mockup's readout uses a second typeface | **Jost with tabular (fixed-width) figures** on the LCD panel, so digits don't jitter. If Jost's figures jitter, we'll pad to a fixed width in code. |
| D3 | Short labels are centered over the round knob | **Left-align labels to the group's left edge** on the 12-column grid. The layout is tested in Figma on Day 5; if it looks wrong, we'll raise it as an exception with a reason. |
| D4 | The v1.0 PRD said "the knob dims" when bypassed | **No dimming below contrast minimums.** Bypass is shown by the switch, its label, the LED and the skyline. The knob and readout stay at full contrast. |
