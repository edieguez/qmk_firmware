# Ideas from other QMK keyboards (survey, November 2026)

Three passes over the QMK tree looked for features and patterns worth borrowing
for this board. Note: the checkout has no community userspaces or personal
keymaps any more (QMK moved them out of the main repo), so the ideas below come
from QMK's own docs, the in-tree community modules under `modules/qmk/`, and
vendor keyboard code. Several famous userspace tricks now live in core; the main
one, Chordal Hold, is already in use here.

Status markers: **done** = implemented on this board, **open** = not done.

## Recommended for this Sofle

Ranked by value against effort, respecting earlier decisions.

1. **Speculative Hold** (`docs/tap_hold.md`) — **done** (`SPECULATIVE_HOLD`
   + `SPECULATIVE_HOLD_FLOW_TERM 150` in `config.h`, `get_speculative_hold()`
   in `keymap.c` for F/J/D/K). The modifier is sent the
   instant a home row mod goes down and withdrawn if it turns out to be a tap.
   Built-in fix for Cmd-click / Shift-click / Shift-select lag with home row
   mods. Config only, plus a short `get_speculative_hold()` to include D and K.
   Flow Tap already guards against stray lone modifiers.
2. **Light the keys that do something on the current layer**
   (`keyboards/decent/tkl/tkl.c`) — **done**, see
   `rgb_matrix_indicators_advanced_user()` in `keymaps/default/keymap.c` and
   `SPLIT_LAYER_STATE_ENABLE` in `config.h`.
3. **Home row mod hint LEDs** (`keyboards/bastardkb/dilemma/3x5_3/3x5_3.c`) —
   open. Flag the eight home row mod keys as modifier LEDs in `keyboard.json`
   and tint them; paint Caps Word on the same loop. Pairs with item 2.
4. **Cmd+Tab on a knob** (`modules/qmk/super_alt_tab`) — open. One keycode
   holds Cmd, taps Tab, releases Cmd a second after the last turn. Fits the free
   LOWER left knob or ADJUST right knob. Enabled by listing the module in a
   `keymap.json`, with one define to use Cmd instead of Alt.
5. **Keylog that clears itself** (`keyboards/tominabox1/le_chiffre/le_chiffre.c`)
   — **done** (`KEYLOG_CLEAR_MS` in `soflecat_macos.c`). Three lines wipe the typed-character log after 30 s idle. Closes
   most of the audit's password-on-OLED finding without removing the feature.
6. **Per-chord Chordal Hold exceptions** (`docs/tap_hold.md`,
   `get_chordal_hold()`) — open. Allow a few same-hand chords (D+C, D+V, D+X,
   D+Z) so left-hand copy/paste works without reaching for K. Choose carefully:
   D+A would turn a fast "da" into Select All.
7. **Idle RGB profile and OLED fade** (`keyboards/nifty_numpad/nifty_numpad.c`,
   `OLED_FADE_OUT`, `RGB_MATRIX_TIMEOUT`) — **done for the OLEDs only**:
   `OLED_FADE_OUT` in `config.h`. `RGB_MATRIX_TIMEOUT` was tried and removed;
   the user wants the lights to stay on when idle.
8. **Quick tap term at zero for the home row** (`QUICK_TAP_TERM_PER_KEY`) —
   open. Tap-then-hold on F gives Shift instead of "fff". Keep the default on
   the Backspace key, which relies on repeat.

## Knobs and screens

What other boards do with the same hardware (two 128x32 OLEDs, 58 LEDs, two
encoders) and how it fits here.

- **Encoder mode cycling** (`keyboards/cannonkeys/lib/satisfaction75`). Click
  advances through modes, OLED shows the mode name. Competes with the per-layer
  knob rule used here; the transferable idea is showing the active knob function
  on the master screen while a layer is held.
- **Knob-driven RGB menu on the second screen**
  (`keyboards/rgbkb/common/common_oled.c`). Click toggles select/adjust, turn
  moves through hue, sat, val, speed, mode, on/off; cursor synced over RPC.
  Would replace the RGB row on ADJUST with one knob. ~70 lines + a split
  transaction. Only worth it if RGB settings are touched often.
- **Hold the knob and turn to change layer** (`keyboards/macrocat/macrocat.c`).
  Fits, but layers are thumb holds here and that works.
- **Luna, the WPM dog** (`keyboards/kikoslab/ellora65`). Sits/walks/runs with
  typing speed, sneaks on Ctrl, jumps on Space, barks on Caps. ~1 KB sprites +
  120 lines. Exactly 32 px wide; a bongocat alternative for the slave screen.
- **Live key-matrix map** (`satisfaction75/satisfaction_oled.c`). One pixel per
  key, lit while held, in a 12x8 box. ~20 lines. The best debugging tool for
  home row mod misfires (shows whether F was still down when / was pressed).
- **Second screen as an RGB panel** (`keyboards/mechboards/common/display_oled.c`).
  The idle half shows mode/hue/sat/val permanently; RGB state is already
  synced, no RPC needed.
- **Hardware-scrolling logo** (`keyboards/controllerworks/mini42`). Free
  CPU-wise, only for an idle state.
- **RPC template for pushing master data to the slave screen**
  (`keyboards/hotdox76v2`, `modules/qmk/split_data_sync`). The pattern for
  anything above that needs the other half to know master state.
- Typing-reactive effects (heatmap, reactive, splash) are built in and need only
  `RGB_MATRIX_KEYPRESSES` (+ `RGB_MATRIX_FRAMEBUFFER_EFFECTS` for the heatmap)
  — **done**: all twelve reactive effects and `typing_heatmap` are enabled, with `SPLIT_TRANSPORT_MIRROR` so both halves
  react. Added later: `RGB_MATRIX_SOLID_REACTIVE_GRADIENT_MODE`. A custom
  per-key-hue `RAINBOW_REACTIVE` effect was tried and removed.

Nothing in the tree has a WPM graph, a game, a host-fed clock, or encoder
acceleration; those would be from scratch.

## Looked at and set aside

- **Combos** for Esc, Tab or brackets: timing-dependent by nature and they
  clash with home row mods on the same keys; Esc/Tab relocation was already
  rejected.
- **Alt-Repeat tables and word-ending keys**: built on Repeat Key, which was
  tried and went unused.
- **Layer Lock on a mod-tap, Tri Layer, one-shot Hyper keys**: variants of
  things removed or declined.
- **Swap Hands**: one-handed mirroring while mousing. Niche.
- **Unicode input**: needs the macOS Unicode Hex Input source, which breaks the
  Option+arrow word jumps used on LOWER.
- **OS detection, Autocorrect, Send String macros, Secure, Raw HID**: no fit for
  a macOS-only board with an IDE/terminal workflow.
- **Dynamic tapping term**: a temporary tuning aid if the HRM test turns up a
  timing problem, then removed.
