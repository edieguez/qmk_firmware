# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

`keyboards/soflecat/` is a QMK keyboard definition for a personal, physically-owned **Sofle Choc** split keyboard: RP2040 Pro Micro clone, 29+29 WS2812 per-key RGB LEDs, one 128x32 OLED and one rotary encoder per half, TRRS split link (`soflecat-specs.md` has the hardware writeup; its firmware half is historical). The user is customizing the layout, not authoring the board — don't attribute maintainership of the Sofle Choc design to the user, and don't rename `keyboard.json`'s `manufacturer`/`keyboard_name` without being asked.

The directory was bootstrapped as a clone of upstream `keyboards/sofle_choc/`, so `keyboard.json`'s "Brian Low" and the readme's build guide links still match upstream; that's expected. It was renamed twice: `soflecat-macos` → `soflecat_macos`, because QMK requires keyboard folder names to match `^[0-9a-z][0-9a-z_/]*$` (hyphens broke `qmk compile` outright), then → `soflecat` in November 2026, because the layout is meant to become OS-adaptive (roadmap item 9 in `lessons-from-lily58cat.md`). The Vial export was renamed with it, to `soflecat.vil`.

The keymap is **macOS-only today**. Linux lives on the sibling board `keyboards/lily58cat/`, and the plan is to merge the two into one firmware that picks the OS at runtime.

**Keyboard-level source file naming is load-bearing.** `builddefs/build_keyboard.mk` only auto-compiles a keyboard-level `.c` whose name exactly matches the folder (`keyboards/<kb>/<kb>.c`), gated by a `wildcard` check — no match, no compile, no error. After the clone the file was still `sofle_choc.c` and was silently excluded from every build: the generic OLED driver still initialised the displays (they flashed on power-up), but `oled_init_kb()`/`oled_task_kb()` were never linked, so the screens went dark right after. If this directory is renamed again, rename `soflecat.c` in the same commit.

## Build & flash

Run from the repo root. `rules.mk` sets `CONVERT_TO = rp2040_ce`, so no `-e CONVERT_TO=...` is needed; the output is `soflecat_default.uf2` in the repo root.

```bash
qmk compile -kb soflecat -km default
make soflecat:default      # equivalent
```

Flashing is UF2 drag-and-drop, not a QMK-driven flash. The user flashes the boards themselves; hand over the command, never copy onto the bootloader drive.

1. Double-tap the reset button on one half. It mounts as a USB drive named `RPI-RP2` (`/Volumes/RPI-RP2` on macOS).
2. `cp soflecat_default.uf2 /Volumes/RPI-RP2/` — the half reboots into the new firmware.
3. Repeat for the other half.

Flash size doesn't matter here (RP2040, ~2 MB). It does on the Lily58.

## Files

- `keyboard.json` — matrix, encoder, split, 42 RGB Matrix animations, per-key LED positions (`rgb_matrix.layout`, keyed by matrix position), the `LAYOUT` macro.
- `soflecat.c` — keyboard-level hooks: both OLEDs, RGB status messages, keylog, and a stuck-USB-suspend recovery (see below).
- `keymaps/default/keymap.c` — layers, custom keycodes, tap dance, tap-hold tuning callbacks, key overrides, Chordal Hold layout, layer-overlay RGB indicator, encoder map.
- `keymaps/default/config.h` — split/OLED/RGB defines and the tap-hold constants.
- `keymaps/default/rules.mk` — `ENCODER_MAP_ENABLE`, `TAP_DANCE_ENABLE`, `WPM_ENABLE`, `CAPS_WORD_ENABLE`, `KEY_OVERRIDE_ENABLE`, `CONVERT_TO = rp2040_ce`, `SRC += lib/bongocat.c`.
- `keymaps/default/lib/` — `bongocat.c/.h` (slave OLED animation, from HorrorTroll) and `glcdfont.c` (stock font plus layer/modifier icons at 0x80+, from splitkb/aurora/sofle_v2; selected through `OLED_FONT_H` in `config.h`).
- Notes: `lessons-from-lily58cat.md` is the **backlog** (items 1–9, with status markers). `qmk-ideas-survey.md` and `qmk-features-reference.md` are research, several items marked done. `soflecat.vil` is not read by the build.

## Key positions

Keys are named by matrix `[row,col]` from `keyboard.json`. Left rows 0–4, right rows 5–9, column 0 is the outer edge on both halves. Row 4/9 holds the knob click at column 5 and the thumbs at columns 4 → 0 (column 4 is the big resting key).

```
│ 0,0 │ 0,1 │ 0,2 │ 0,3 │ 0,4 │ 0,5 │             │ 5,5 │ 5,4 │ 5,3 │ 5,2 │ 5,1 │ 5,0 │
│ 1,0 │ 1,1 │ 1,2 │ 1,3 │ 1,4 │ 1,5 │             │ 6,5 │ 6,4 │ 6,3 │ 6,2 │ 6,1 │ 6,0 │
│ 2,0 │ 2,1 │ 2,2 │ 2,3 │ 2,4 │ 2,5 │             │ 7,5 │ 7,4 │ 7,3 │ 7,2 │ 7,1 │ 7,0 │
│ 3,0 │ 3,1 │ 3,2 │ 3,3 │ 3,4 │ 3,5 │ 4,5 │ │ 9,5 │ 8,5 │ 8,4 │ 8,3 │ 8,2 │ 8,1 │ 8,0 │
            │ 4,0 │ 4,1 │ 4,2 │ 4,3 │ 4,4 │ │ 9,4 │ 9,3 │ 9,2 │ 9,1 │ 9,0 │
BASE today: │PgDn │PgUp │Globe│Lower│Space│ │Enter│Bs/Up│  =  │Home │ End │
```

## The layout today

Four layers: `_BASE`, `_LOWER`, `_UPPER`, `_ADJUST`. The ASCII diagram above each layer in `keymap.c` is the reference; keep it in sync with the array.

- **BASE:** QWERTY with home row mods — `LALT_T(A)`, `LCTL_T(S)`, `LGUI_T(D)`, `LSFT_T(F)` and the mirror on J K L ;. `2,0` is `TD(TD_CAPS)`: one tap Caps Word (or Caps Lock off if it's on), two taps Caps Lock. `3,0` is `[`, `8,0` is `ADJ_RBRC` = `LT(_ADJUST, KC_RBRC)`. Thumbs as in the diagram; `9,3` is `UPP_BSPC` = `LT(_UPPER, KC_BSPC)`. Knob clicks: Mute (left), Play (right). `4,2` is `MACOS_GLOBE`, a custom keycode sending consumer usage `AC_NEXT_KEYBOARD_LAYOUT_SELECT` (QMK has no Globe keycode).
- **LOWER** (left thumb hold): arrows on H J K L, word jumps `A(KC_LEFT/RIGHT)` on Y and O, line start/end `G(KC_LEFT/RIGHT)` on `5,0`/`6,0`, document top/end `G(KC_UP/DOWN)` on `7,0`/`8,0`. `` ` `` on Esc, ¡ on 1, ü on U, ñ on N, Delete on M, ¿ on /. Enter sends Shift+Enter, Backspace sends Option+Backspace. Right knob jumps by word.
- **UPPER** (held Backspace): F1–F5 on the number row, F6–F10 on Q–T, F11/F12 on D/F. Left knob zooms (`G(-)`/`G(=)`), click `G(0)` resets.
- **ADJUST** (held `]`): RGB toggle/mode/hue on the left number row, sat/bri/speed on the right. Mission Control on W, Dictation (`LCAG(KC_D)`) on R, Do Not Disturb (`LCAG(KC_N)`) on the left knob click; those two chords must be bound in System Settings. Left knob: previous/next track.
- **ñ / ü:** `ES_NTIL`/`ES_UDIA` custom keycodes. macOS Option+N / Option+U is a dead key, so `tap_mac_accent()` taps it with Shift removed, then the letter with Shift restored (or added if Caps Word is on). ¿ and ¡ are plain `A(S(KC_SLSH))` and `A(KC_1)`.
- **Key overrides:** Shift+Esc → `~` (Grave Escape was rejected: Cmd+Esc became Cmd+`, which macOS eats, so it couldn't cancel Cmd+Tab) and Option+' → `` ` ``.
- **Encoder map** per layer, in `keymap.c`. BASE: left knob F14/F15 (macOS screen brightness, confirmed working), right knob volume.

### Tap-hold tuning (hard-won, don't undo casually)

`config.h`: `TAPPING_TERM 200`, `PERMISSIVE_HOLD` (per key), `CHORDAL_HOLD`, `FLOW_TAP_TERM 150`, `SPECULATIVE_HOLD` with `SPECULATIVE_HOLD_FLOW_TERM 150`. The callbacks in `keymap.c` single out the four Shift/Cmd keys (`LSFT_T(F)`, `RSFT_T(J)`, `LGUI_T(D)`, `RGUI_T(K)`):

- `get_tapping_term()`: +150 ms for those four (chords at speed have almost no overlap; +80 wasn't enough; Hold On Other Key Press fired on bigrams like "fi").
- `get_permissive_hold()`: those four plus `ADJ_RBRC` only. On Alt/Ctrl it turned cross-hand rolls into holds ("also" → "aso").
- `get_flow_tap_term()`: 0 for those four, otherwise Flow Tap settled them as taps before Chordal Hold saw the next key (broke fast Shift+/ and Cmd+V).
- `get_speculative_hold()`: those four only, so Shift/Cmd-click is instant. Ctrl stays out because Ctrl+click is right-click on macOS.
- `chordal_hold_layout`: thumbs and knob clicks are `*`, or Cmd+Space / Cmd+Enter settle as taps.
- `UPP_BSPC` is kept out of Permissive Hold so a backspace rolled into the next letter stays a tap.
- `caps_word_press_user()` returns true for `TD(TD_CAPS)` without breaking the word; otherwise the dance would cancel Caps Word and then turn it back on.
- A global `TAP_CODE_DELAY` was tried and removed: it stalled the knobs.

### Known Adjust problem

`ADJ_RBRC` with Permissive Hold loses `]` when it overlaps a key on the other hand that is pressed and released first (`a[i] = x` typed fast). Backlog item 1 moves Adjust to a plain `MO()` on a left thumb.

## OLEDs and RGB (`soflecat.c`)

- Master OLED is portrait (`OLED_ROTATION_270`, 5 columns × 16 rows): layer icon (rows 0–2), GUI/Alt and Ctrl/Shift icons (rows 4–7, lit from `get_mods() | get_oneshot_mods() | get_speculative_mods()`), `CAPS` and `WORD` flags (rows 9–10), and a 20-character keylog (rows 12–15) that only logs keys with an obvious single character, logs the tap keycode of mod-taps and layer-taps, and clears after 30 s idle (`KEYLOG_CLEAR_MS`). Slave OLED: bongocat with WPM (`SPLIT_WPM_ENABLE`).
- RGB changes (toggle, mode name, hue, sat, bri, speed) replace the status screen for 1.5 s. `poll_rgb_status()` diffs the RGB state each frame; sat/val/speed at their limits don't change, so `process_record_kb()` flags the `RM_*` presses (they're consumed before `post_process_record_kb()`). `oled_write()` only touches the cells it draws, so the screen is cleared when switching between message and status and when the message text changes.
- `rgb_matrix_indicators_advanced_user()` in `keymap.c` lights the keys that do something on the held layer (Lower cyan, Upper green, Adjust orange) and blanks the rest, except on Adjust where the running effect stays visible. Needs `SPLIT_LAYER_STATE_ENABLE`; `SPLIT_TRANSPORT_MIRROR` lets the slave react to the master's presses.
- `RGB_MATRIX_TIMEOUT` was tried and removed: the user wants the lights on when idle. `RGB_MATRIX_MAXIMUM_BRIGHTNESS 100`: higher can brown out the controller.
- `OLED_FADE_OUT`, `SPLIT_OLED_ENABLE` and `SPLIT_ACTIVITY_ENABLE` keep both screens sleeping and waking together.

## Stuck USB suspend

If USB stays suspended with no bus reset (after a host restart), QMK's suspend loop never exits and only a replug helps. `suspend_power_down_kb()` resets the MCU once after 60 s of suspend. A token in a RAM section that survives reset (`.ram0.stuck_suspend_recovered`) stops it resetting every minute during a real long sleep; `suspend_wakeup_init_kb()` clears it.

## Working rules

- Only edit this folder (and `keyboards/lily58cat/`). QMK core is off limits even for real bugs; put workarounds in `soflecat.c`, `rules.mk` or new files here.
- One step at a time: implement, compile, hand over the flash command, wait for the test result, then commit. Commit messages are one plain subject line, changes joined with `;`, no body, no attribution line.
- clangd shows bogus errors here because it doesn't know QMK's include paths. Trust `qmk compile`.
- Stale build cache after moving files: delete `.build/obj_soflecat_default/`.
- Before changing a key, check `lessons-from-lily58cat.md`, which records decisions already made and the reasons; and `qmk-ideas-survey.md`'s "Looked at and set aside" list, so rejected ideas (Combos, Repeat Key, Unicode input, Grave Escape, Auto Shift) aren't re-proposed.
