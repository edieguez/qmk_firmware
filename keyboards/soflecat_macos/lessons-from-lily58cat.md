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
BASE today: │PgDn │PgUp │Globe│Lower│Space│ │Enter│Bs/Up│  =  │Home │ End │
```

## Part 1: lessons to port

1. **Adjust on a thumb, and a rebuilt Adjust** — open.
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

2. **Lower's outer column: line start/end and pages** — open.
   - **Lily layout:** `5,0` line start, `6,0` line end, `7,0` PgUp, `8,0`
     PgDn, with the document jumps dropped.
   - **Here:** `5,0` `G(KC_LEFT)` and `6,0` `G(KC_RIGHT)` already are line
     start/end. Change `7,0` and `8,0` from document top/end (`G(KC_UP)`,
     `G(KC_DOWN)`) to `KC_PGUP`/`KC_PGDN`.
   - **Frees BASE thumbs:** with PgUp/PgDn on Lower, `4,0` PgDn and `4,1`
     PgUp come free. Home/End on `9,1`/`9,0` are *not* duplicates on macOS
     (they scroll the document instead of moving to line start/end), so keep
     them unless they're unused.

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

4. **A screenshot key** — open.
   - The Lily has Print Screen on the far left thumb (`4,1` there). The macOS
     equivalent is Cmd+Shift+5 (`G(S(KC_5))`, the capture panel with area,
     window and recording), or Cmd+Shift+4 (`G(S(KC_4))`) for straight area
     capture.
   - Candidate slot: a far left thumb freed by item 2 (`4,0` or `4,1`).

## Part 2: more improvements, to discuss

5. **Prune the RGB Matrix effect list** — open, to discuss.
   `keyboard.json` enables 42 animations, many near-duplicates: `band_sat` /
   `band_val`, `band_pinwheel_sat` / `band_pinwheel_val`, `band_spiral_sat` /
   `band_spiral_val`, seven `cycle_*` variants, and eight `solid_reactive_*`
   variants. On the Lily strip, look-alike effects made Mode± tedious. The
   same applies here even though flash isn't a concern. Pick a short list of
   effects that look clearly different, and keep one or two reactive effects
   and the typing heatmap if they're used.

6. **Refresh the stale notes** — open, to discuss. Both files are untracked
   and have drifted from `keymap.c`:
   - **`CLAUDE.md`:** describes a number-row tap dance (digit/F-key) and a
     "nav/Fn layer pair", and says `rules.mk` only has `ENCODER_MAP_ENABLE`.
     Today there's no number-row tap dance, the layers are Lower/Upper/Adjust,
     and `rules.mk` also enables Tap Dance, WPM, Caps Word and Key Overrides.
   - **`quantum-keys-todo.md`:** lists `QK_GESC` on the top-left key, `OSM`
     Shift keys, `QK_BOOT` on Adjust, and Option+Minus/I/O overrides. The
     keymap has plain `KC_ESC`, plain mods, no `QK_BOOT`, and only the
     Shift+Esc → `~` and Option+' → `` ` `` overrides.

7. **Use the `[row,col]` notation in this board's notes** — open, to
   discuss. Add the diagram above to `CLAUDE.md`, so layout requests and
   reviews can name keys the way the Lily review did.

8. **Reassign the freed thumbs** — open, to discuss. Once items 1, 2 and 4
   are in, decide what each left thumb gets. Candidates: Adjust (next to
   Lower, as on the Lily), the screenshot key, Delete (if it leaves Lower for
   ñ), and the Globe key `4,2` (keep, move or drop). On the Lily the thumbs
   ended up as Print Screen, Adjust, Lower, Space and Enter,
   Backspace/Upper, `=`, Super.

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
