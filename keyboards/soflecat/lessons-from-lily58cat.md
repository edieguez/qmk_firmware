# Lessons from the lily58cat port (November 2026)

The Sofle keymap was ported to a stock Lily58 running Linux
(`keyboards/lily58cat/` on the `lily58` branch), then reviewed layer by layer
on the real board. Some of the decisions made there fix problems this board
still has; others are just preferences worth carrying over. This file only
records them. Nothing here is implemented yet.

Status markers: **open** = not done, **done** = implemented on this board.
Items in Part 2 are also marked **to discuss**: they still need a decision.

## Key positions

Positions are matrix `[row,col]` from `keyboard.json`. Left rows are 0–4,
right rows 5–9, and column 0 is the outer edge on both halves. Row 4/9 holds
the inner key at column 5 (the knob click here) and the thumbs: column 4 is
the big resting key, out to column 0. The Sofle has one more thumb per side
than the Lily58 (`4,0`/`9,0`).

```
│ 0,0 │ 0,1 │ 0,2 │ 0,3 │ 0,4 │ 0,5 │             │ 5,5 │ 5,4 │ 5,3 │ 5,2 │ 5,1 │ 5,0 │
│ 1,0 │ 1,1 │ 1,2 │ 1,3 │ 1,4 │ 1,5 │             │ 6,5 │ 6,4 │ 6,3 │ 6,2 │ 6,1 │ 6,0 │
│ 2,0 │ 2,1 │ 2,2 │ 2,3 │ 2,4 │ 2,5 │             │ 7,5 │ 7,4 │ 7,3 │ 7,2 │ 7,1 │ 7,0 │
│ 3,0 │ 3,1 │ 3,2 │ 3,3 │ 3,4 │ 3,5 │ 4,5 │ │ 9,5 │ 8,5 │ 8,4 │ 8,3 │ 8,2 │ 8,1 │ 8,0 │
            │ 4,0 │ 4,1 │ 4,2 │ 4,3 │ 4,4 │ │ 9,4 │ 9,3 │ 9,2 │ 9,1 │ 9,0 │
BASE today: │ Lg1 │ Lg2 │Globe│Lower│Space│ │Enter│Bs/Up│  =  │Shot │ Adj │
```

## Part 1: lessons to port

1. **Adjust on a thumb, and a rebuilt Adjust** — done (2026-11-17): plain
   `KC_RBRC` on `8,0`, `ADJ_RBRC` dropped from `get_permissive_hold()`,
   `MO(_ADJUST)` on the right outer thumb `9,0`. The layer itself wasn't
   rebuilt: held with the right thumb, its left-hand keys (RGB, Mission
   Control, Dictation, the tapping-term keys) are already under the free
   hand. The original notes follow.
   - **Problem:** `8,0` is `LT(_ADJUST, KC_RBRC)` (`ADJ_RBRC`) with
     Permissive Hold. If `]` overlaps a left-hand key or a thumb that is
     pressed *and released* before `]` comes up, it becomes Adjust plus that
     key and the `]` is lost (`a[i] = x` typed fast). Same-hand rolls such as
     `];` are safe because Chordal Hold keeps `]` a tap.
   - **Lily fix:** a plain `KC_RBRC` on `8,0`, `ADJ_RBRC` dropped from
     `get_permissive_hold()`, and Adjust as a plain `MO(_ADJUST)` on a left
     thumb, next to Lower. Here that would be a thumb freed by item 2 (see
     item 8).
   - **Adjust layout used on the Lily**, held with the left thumb so it all
     sits under the right hand:
     - RGB toggle on `0,0` (easy to remember: top-left corner = lights).
     - RGB settings as vertical pairs, + on the number row above − on the
       top row: Mode `5,5`/`6,5`, Speed `5,4`/`6,4`, Hue `5,3`/`6,3`,
       Saturation `5,2`/`6,2`, LED brightness `5,1`/`6,1`.
     - The outer column `5,0`–`8,0` for levels, + above −: on the Lily
       that's Vol+, Vol−, screen Bri+, Bri−.
     - Prev/Play/Next on J K L (`7,4`/`7,3`/`7,2`), Mute on an inner key.
   - **Sofle differences to keep:** the knobs already do volume (right) and
     screen brightness (left, F14/F15) on BASE, and Prev/Next on Adjust. So
     the levels column and the media keys are optional here. Mission Control,
     Dictation and Do Not Disturb (today on `1,2`, `1,4` and `4,5`) need new
     slots on the left hand. The RGB status messages on the OLED keep working,
     because the keycodes don't change.

2. **Lower's outer column: Home, End, PgUp, PgDn** — done (2026-11-17).
   `5,0` Home, `6,0` End, `7,0` PgUp, `8,0` PgDn, as on the Lily; the
   Cmd+arrow line and document jumps were dropped (the home row mods give
   Cmd+arrow anyway). The BASE thumbs `4,0`, `4,1`, `9,1`, `9,0` are now
   `KC_NO`, waiting for item 8.

3. **ñ, ¿, ¡ on Lower** — open.
   - **Positions as on the Lily:** Lower+N `8,5` for ñ, Lower+/ `8,1` for ¿,
     Lower+1 `0,1` for ¡. `8,5` is Lower's Delete today, so Delete would move
     (for example to a freed thumb).
   - **macOS sequences:**
     - **ñ:** Option+N (dead tilde), then N. Lift Shift for the Option+N tap,
       so held Shift (or Caps Word's weak Shift) only applies to the second
       tap and gives Ñ. Two taps means a custom keycode handled in
       `process_record_user()`. Add it to `caps_word_press_user()` with a
       weak Shift, as `ES_NTIL` is on the Lily.
     - **¿:** Option+Shift+/ (`A(S(KC_SLSH))`).
     - **¡:** Option+1 (`A(KC_1)`).
   - **Reference:** `ES_NTIL`, `ES_IQUE`, `ES_IEXL` and
     `caps_word_press_user()` in `keyboards/lily58cat/keymaps/default/keymap.c`.
     That version sends AltGr for Linux US International, so the sending part
     differs.

4. **A screenshot key** — done (2026-11-19): a tap dance on the right
   thumb `9,1`, the last blank key. Tap Cmd+Shift+4 (area), double tap
   Cmd+Shift+3 (whole screen), hold Cmd+Shift+5 (capture panel).
   - The Lily has Print Screen on the far left thumb (`4,1` there). The macOS
     equivalent is Cmd+Shift+5 (`G(S(KC_5))`, the capture panel with area,
     window and recording), or Cmd+Shift+4 (`G(S(KC_4))`) for straight area
     capture.

## Part 2: more improvements, to discuss

5. **Prune the RGB Matrix effect list** — open, to discuss.
   `keyboard.json` enables 42 animations, many near-duplicates: `band_sat` /
   `band_val`, `band_pinwheel_sat` / `band_pinwheel_val`, `band_spiral_sat` /
   `band_spiral_val`, seven `cycle_*` variants, and eight `solid_reactive_*`
   variants. On the Lily strip, look-alike effects made Mode± tedious. The
   same applies here even though flash isn't a concern. Pick a short list of
   effects that look clearly different, and keep one or two reactive effects
   and the typing heatmap if they're used.

6. **Refresh the stale notes** — done. `CLAUDE.md` was rewritten against
   the current `keymap.c`, `rules.mk` and `soflecat.c`; `quantum-keys-todo.md`
   (Grave Escape, OSM shifts, `QK_BOOT`, Option+Minus/I/O overrides, none of
   which exist) was deleted.

7. **Use the `[row,col]` notation in this board's notes** — done. The
   diagram above is in `CLAUDE.md`.

8. **Reassign the freed thumbs** — done (2026-11-17). `9,0` is Adjust,
   `4,0`/`4,1` the login macros (item 13), `4,2` Globe and `9,1` the
   screenshot panel (item 4). No thumb is free. On the Lily the thumbs
   ended up as Print Screen, Adjust, Lower, Space and Enter,
   Backspace/Upper, `=`, Super.

## Roadmap

9. **One layout that adapts to the computer** — planned, not started.
   When plugged into a PC, the board uses the Linux layout; when plugged
   into a Mac, the macOS layout. One firmware, no reflashing.
   - **Detecting the computer:** QMK's OS detection (`OS_DETECTION_ENABLE`,
     `detected_host_os()`) guesses the host from how it sets up the USB
     connection. It's a heuristic: docks, hubs and KVM switches can fool it,
     and it can answer "unsure". So there also has to be a manual override
     key (on Adjust), remembered across power-off. Detection happens only on
     the USB half; `SPLIT_DETECTED_OS_ENABLE` shares it with the other half.
   - **What has to switch:** the same list as lily58cat's two keymaps. Their
     `LILY58CAT_MACOS` block in `keyboards/lily58cat/keymaps/default/keymap.c`
     is the reference:
     - home row mods (Ctrl vs Cmd on D/K, with the tuning following them)
     - word and line jumps, word delete
     - Super vs Globe
     - screen brightness keys (`KC_BRIU`/`KC_BRID` vs F15/F14)
     - ñ ü ¿ ¡ (AltGr vs Option sequences)
     - `` ` `` and `~` (symbol + space on Linux US International, plain on
       macOS)
     - the Caps Lock hold (macOS only)
     - the Mac-only Adjust keys (Mission Control, Launchpad, Dictation, Do
       Not Disturb)
   - **Likely approach:** build both key sets into the firmware and choose at
     runtime: separate base layers per OS (`default_layer_set()`) for the
     layer keys, and a runtime check in `process_record_user()` for the
     sequences. Flash isn't a concern on the RP2040.
   - **Prerequisite:** the Sofle is macOS-only today. Its Linux side would
     come from lily58cat's default keymap.
   - **Lily58:** the same idea probably won't fit there. Only about 500–750 B
     are free, and two key sets plus OS detection need more. It stays on two
     firmwares (`-km default` / `-km macos`) unless something is cut.

## Part 3: keys reachable another way

10. **Drop `` ` `` from Lower+Esc (`0,0`)** — done (2026-11-17). Option+'
    gives `` ` `` through `grave_key_override`, and Shift+Esc gives `~`.
    Delete moved from Lower+M (`8,4`) into the freed slot.

11. **Other duplicates of the same kind** — open, to discuss (found in a
    review on 2026-11-17, not yet decided):
    - **Lower's right knob** sends Option+Left/Right, the same as Lower+Y
      (`6,5`) and Lower+O (`6,2`). One of the two is redundant.
    - **Lower+Enter = Shift+Enter** (`9,4`). Home row Shift (F or J) plus
      Enter does the same, and Permissive Hold makes that chord fast.
    - **Lower+Backspace = Option+Backspace** (`9,3`). Home row Alt (A) plus
      Backspace does the same, but Alt has no Permissive Hold, so A must be
      held a full `TAPPING_TERM` first; the Lower key is quicker.

## Part 4: tuning from the keyboard

12. **Home row mod tapping terms adjustable from Adjust, saved to EEPROM**
    — done (2026-11-17). On Adjust, tap A, S, D or F to pick
    Alt/Ctrl/Cmd/Shift, then turn the left knob (5 ms a detent); G shows all
    four on the OLED, B resets them (Alt/Ctrl 200, Cmd/Shift 350). The
    values live in the user EEPROM block; the selection starts at Shift.
    Previous/next track moved to the right knob on Adjust. Flow Tap stays
    compile-time, and the Speculative Hold flow term can't be made
    adjustable without editing core.

13. **Two login macros, recorded on the keyboard and saved to EEPROM** —
    done (2026-11-17). `4,0`/`4,1` on BASE type them (tap: fast; with a
    home row Shift held: at the recorded pace). Adjust + the same thumb
    records; the thumb again stops and saves. 32 keys per slot with
    modifiers and the gap before each key. QMK's Dynamic Macros weren't
    used: their buffer is RAM only and keeps no timing.

## Not applicable here

Recorded so they aren't re-investigated:

- **rgblight split fix (#26481) and the speed quirk:** this board uses RGB
  Matrix, not rgblight.
- **Flash-budget tricks** (no `snprintf` or floats, a 3-frame bongocat): the
  RP2040 has about 2 MB of flash.
- **US International workarounds** (Left Alt on `;`, symbol + space for
  `` ` ``/`~`, AltGr keys for ñ/¿/¡): macOS's US layout has no AltGr and none
  of those dead keys.
- **A lone Super key:** macOS doesn't use a lone Cmd tap.
