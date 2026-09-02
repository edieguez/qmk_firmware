# QMK Features & Special Keys Reference

A survey of QMK's feature set, done while weighing what (if anything) to assign to `soflecat_macos`'s three thumb keys that started out permanently unassigned. Two are still dead (matrix `[4,1]`, `[4,2]`); the third of the original trio, `[9,0]`, now holds `_ADJUST` (moved there from `[9,1]`, which is the dead one instead now) — see the now-partially-resolved item in `CLAUDE.md`'s Review findings. Kept as a standalone reference for future keymap work, not just this one decision.

Sourced from `docs/features/` (49 feature docs) and the keycode ranges in `quantum/keycodes.h`, cross-checked against what this keyboard currently enables (`keyboard.json`, `keymaps/default/rules.mk`, `config.h`, `keymap.c`). This is a point-in-time snapshot — re-verify against those files before relying on it, since upstream QMK's feature set evolves.

## Already in use on this board

| Feature | Where configured | Note |
|---|---|---|
| Layer momentary-hold (`MO`) | `keymap.c` | Nav/Fn/Adjust are all held, not toggled |
| Tap Dance | `rules.mk` (`TAP_DANCE_ENABLE`), `keymap.c` | `TD_CAPS`: tap = Caps Word toggle, double-tap = Caps Lock |
| Key Overrides | `rules.mk` (`KEY_OVERRIDE_ENABLE`), `keymap.c` | 4 overrides: `RAlt+Backspace`→Delete (right-Alt only, not either Alt), `Shift+Esc`→`~`, `Option+Minus`→`=`, `Option+'`→`` ` `` |
| Caps Word | `rules.mk` (`CAPS_WORD_ENABLE`) | Reached via `TD_CAPS` single tap |
| WPM | `rules.mk` (`WPM_ENABLE`), `config.h` (`SPLIT_WPM_ENABLE`) | Drives bongocat animation speed + OLED readout |
| Encoders | `keyboard.json`, `rules.mk` (`ENCODER_MAP_ENABLE`) | Volume/brightness on `_BASE`, Hue/Speed on `_ADJUST` |
| RGB Matrix | `keyboard.json`, `soflecat_macos.c` | Full per-key addressable RGB + OLED status feedback loop |
| Split transport | `config.h` | `SPLIT_USB_DETECT`, `SPLIT_WATCHDOG_ENABLE`, `SPLIT_ACTIVITY_ENABLE`, `SPLIT_TRANSPORT_MIRROR` |
| Bootmagic | `keyboard.json` (`bootmagic: true`) | Also has a dedicated `QK_BOOT` key on `_ADJUST` |

## Enabled but unused

> [!NOTE]
> **Mouse Keys** — `mousekey: true` is set in `keyboard.json`, but no `KC_MS_*` keycode appears anywhere in `keymap.c`. The feature is compiled in and doing nothing.

## Individual special keycodes

Not full subsystems — single quantum keycodes that don't need a `docs/features/` page's worth of setup, but come up often enough to be worth knowing individually.

### Grave Escape (`QK_GESC`) — already considered and rejected here

**How it actually works** (`quantum/process_keycode/process_grave_esc.c`): on key-down, it reads whichever modifiers are currently held. If Shift or GUI is among them, it sends `` ` `` (grave); otherwise it sends `Escape`. It remembers which one it chose so key-up releases the correct key even if modifiers changed mid-hold. Four optional `config.h` defines can force plain Escape regardless of the Shift/GUI check: `GRAVE_ESC_ALT_OVERRIDE`, `GRAVE_ESC_CTRL_OVERRIDE`, `GRAVE_ESC_GUI_OVERRIDE`, `GRAVE_ESC_SHIFT_OVERRIDE` — each one forces Escape when its named modifier is held, instead of the default grave-on-Shift/GUI behavior.

**Why this board doesn't use it, and a detail worth knowing if it's ever reconsidered**: `keymap.c`'s `tilde_esc_override` comment explains that macOS intercepts `Cmd+` ` ``` system-wide as "move focus to next window" before it ever reaches an app, so `QK_GESC`'s GUI-held case (sending grave while Cmd is down) never actually worked here — including failing to cancel `Cmd+Tab`'s app switcher. The board now uses plain `KC_ESC` plus a single Key Override (`Shift+Esc` → `~`) instead. Worth noting: `GRAVE_ESC_GUI_OVERRIDE` — one of the four defines above — would force `QK_GESC` to send plain `Escape` (not grave) whenever GUI is held, which is exactly the outcome this board built by hand with a separate mechanism. A single `#define` could have gotten the same result while keeping `QK_GESC` itself in the keymap.

### Magic Keycodes — persist to EEPROM, not just runtime

Confirmed in `quantum/process_keycode/process_magic.c`: every Magic Keycode press updates an in-memory `keymap_config` struct, immediately calls `eeconfig_update_keymap()` to persist it to EEPROM, then calls `clear_keyboard()` to release any keys still physically held (remapping which physical key sends which modifier mid-press could otherwise leave a "stuck" modifier in the next report). **These settings survive power cycling** — unlike Dynamic Macros, which are explicitly volatile. No `_ENABLE` flag is needed; `process_magic.c` is already compiled into every build, confirmed in this board's own `.build/` output.

| Keycode | What it actually flips |
|---|---|
| `CL_SWAP` / `CL_NORM` / `CL_TOGG` | `keymap_config.swap_control_capslock` — set true / set false / flip |
| `EC_SWAP` / `EC_NORM` / `EC_TOGG` | `keymap_config.swap_escape_capslock` |
| `CG_SWAP` / `CG_NORM` / `CG_TOGG` | Both `swap_lctl_lgui` and `swap_rctl_rgui` together (set / clear / flip) |
| `AG_SWAP` / `AG_NORM` / `AG_TOGG` | Both `swap_lalt_lgui` and `swap_ralt_rgui` together |
| `CG_LSWP`/`CG_LNRM`, `CG_RSWP`/`CG_RNRM` | Left-only or right-only Ctrl/GUI swap (no toggle variant exists) |
| `AG_LSWP`/`AG_LNRM`, `AG_RSWP`/`AG_RNRM` | Left-only or right-only Alt/GUI swap (no toggle variant exists) |
| `GU_OFF` / `GU_ON` / `GU_TOGG` | `keymap_config.no_gui` — disables both GUI keys outright, not a swap, an actual kill switch |
| `GE_SWAP` / `GE_NORM` | `keymap_config.swap_grave_esc` — swaps the physical Grave and Escape keycodes outright; simpler than `QK_GESC`, no Shift/GUI special-casing at all |
| `BS_SWAP` / `BS_NORM` / `BS_TOGG` | `keymap_config.swap_backslash_backspace` |
| `NK_ON` / `NK_OFF` / `NK_TOGG` | `keymap_config.nkro` — also calls `clear_keyboard()` first specifically to avoid stuck keys mid-rollover-mode-switch |
| `EH_LEFT` / `EH_RGHT` | Which split half is "left" for `EE_HANDS`, via `eeconfig_update_handedness()` |

Given this board already gets flexible modifier placement from home-row mods, most of these swap/remap keycodes would be redundant with what the keymap already does directly — but `NK_TOGG` is a genuinely useful troubleshooting key if a particular host/BIOS ever chokes on NKRO, since it lets you test without a recompile.

### Command (`COMMAND_ENABLE`) — the older, bundled version of the above

Hold a chord (default Left Shift+Right Shift, checked via the `IS_COMMAND()` macro) then a single letter to trigger one of a fixed menu of actions: print the running QMK version or full keyboard status to the console, clear the EEPROM, jump to the bootloader, toggle NKRO, toggle debug/matrix/keyboard/mouse debugging, or even fully lock the keyboard so nothing types. Full default key table in `docs/features/command.md`. Its own docs note it's largely superseded by the individual Magic Keycodes above (no shift-chord to remember or accidentally trigger); mostly of historical interest now.

### EEPROM Clear (`EE_CLR` / `QK_CLEAR_EEPROM`)

Confirmed in `quantum/process_keycode/process_quantum.c`: pressing this calls `eeconfig_disable()` (invalidates the stored config so it reinitializes to firmware defaults on next boot) and then immediately `soft_reset_keyboard()` — so the keyboard briefly reboots as part of pressing this key. Resets RGB mode/color/brightness, default layer, and anything else persisted via EEPROM. Distinct from `QK_BOOT` (already on `_ADJUST` — that just enters the flashing bootloader and does *not* touch EEPROM at all). **Good, simple candidate** for one of the 3 dead keys: a dedicated "wipe my RGB experiments and start over" key that needs no recompile and no reaching for the reset button.

### Debug Toggle (`DB_TOGG` / `QK_DEBUG_TOGGLE`)

Confirmed in the same file: flips a plain in-memory `debug_enable` boolean and prints `DEBUG: enabled.`/`DEBUG: disabled.` to the console (viewable via `qmk console`). **Not persisted** — resets to off on every reboot, unlike the Magic Keycodes above. Useful while actively troubleshooting firmware behavior without hardcoding `#define DEBUG_ENABLE` and reflashing.

### Reboot (`QK_REBOOT`) — a quieter sibling of `QK_BOOT`

Calls `soft_reset_keyboard()` rather than `reset_keyboard()` — restarts the MCU and re-runs the current firmware immediately, without dropping into the UF2 bootloader / mass-storage mode the way `QK_BOOT` does. Useful for "did my last EEPROM write actually take" checks without needing to re-flash anything afterward.

### Make/Flash from keyboard (`QK_MAKE`) — needs `ENABLE_COMPILE_KEYCODE = yes`

Types out `qmk compile -kb <this keyboard> -km <this keymap>` (or `qmk flash ...` if Shift is held) as literal keystrokes into whatever window currently has focus on the host, then presses Enter — and if both Shift and Ctrl are held, additionally reboots the keyboard into its bootloader right after. **Safety note**: this is just keystroke injection with no awareness of what's focused — pressed in the wrong window, it types a stray command line into whatever you were actually doing instead of a terminal.

### Locking Caps/Num/Scroll Lock (`KC_LCAP`/`KC_LNUM`/`KC_LSCR`)

Only relevant for keyboards with physically mechanical locking switches under those keys. Not applicable here.

## Layer mechanics not yet used

| Keycode / technique | Requires | Description | Candidacy for the dead keys |
|---|---|---|---|
| `OSL(layer)` | — (core) | One-shot layer: tap once, only the *next* keypress uses that layer, then it reverts | Good — complements existing held `MO()` layers for single-key nav taps |
| `TG(layer)` | — (core) | Toggle a layer on/off persistently | Possible, but momentary already covers the common case |
| `TT(layer)` | — (core) | Hybrid: hold = momentary, repeated taps = toggle on/lock | Niche |
| `TO(layer)` | — (core) | Jump straight to a layer, bypassing the base layer | Rarely needed with only 4 layers |
| `DF(layer)` / `QK_PERSISTENT_DEF_LAYER` | — (core) | Set/persist the default base layer | Not relevant — only one base layer exists |
| Tri Layer (`update_tri_layer_state()`) | — (technique, not a flag) | Holding two layers simultaneously (e.g. Nav+Fn) auto-activates a third (e.g. Adjust) | Interesting alternate path to `_ADJUST`, but a bigger structural change than "assign a dead key" |
| `QK_LLCK` (Layer Lock) | `LAYER_LOCK_ENABLE = yes` | Press once to "lock" whatever layer(s) a held `MO()` currently has active, so it stays on hands-free until pressed again | **Strong candidate** — pairs directly with the existing momentary-only Nav/Fn/Adjust setup |

## Typing assist & macros

| Feature | Requires | Description | Notes |
|---|---|---|---|
| Dynamic Macros | `DYNAMIC_MACRO_ENABLE = yes` | Record/replay key sequences at runtime, lost on reboot | Minimum 2 keys for one slot (`DM_REC1` doubles as start/stop, `DM_PLY1` plays); up to 5 keys (`DM_REC1/2`, `DM_PLY1/2`, `DM_RSTP`) for two slots + dedicated stop. **128 keypresses shared** across however many slots are used (`DYNAMIC_MACRO_SIZE`, adjustable) |
| Leader Key | `LEADER_ENABLE = yes` | Press `QK_LEAD`, then a short key sequence, to trigger a custom action | **Considered and declined** — see the checklist below. Was a gateway-to-unlimited-sequences pitch, but it was an unrequested suggestion the user didn't want |
| Repeat Key | `REPEAT_KEY_ENABLE = yes` | `QK_REP` repeats whatever key was last pressed | Good for e.g. repeating Enter after a search, or an arrow press, without moving hands back |
| Alternate Repeat Key | `REPEAT_KEY_ENABLE = yes` | `QK_AREP` — like Repeat Key, but definable keys can have an "opposite" action (e.g. after `(`, sends `)`) | More setup (a `get_alt_repeat_key_keycode_user()` table), more payoff once tuned |
| Combos | `COMBO_ENABLE = yes` | Chord multiple *ordinary* keys together for a different output (distinct from Key Overrides, which need a held modifier) | Not tied to a specific dead key — triggers off existing keys |
| Auto Shift | `AUTO_SHIFT_ENABLE = yes` | Hold a key slightly longer instead of pressing Shift | Likely conflicts with home-row-mod Shift already in use |
| Autocorrect | `AUTOCORRECT_ENABLE = yes` | On-device typo correction from a built-in dictionary, toggled via `AC_TOGG` | Real feature, but needs a dictionary build step — bigger lift than a key assignment |
| Space Cadet | `SPACECADET_ENABLE = yes` | Tapping Shift/Ctrl/Alt/GUI alone (not held) sends parens/brackets | Likely conflicts with home-row mods on the same physical keys |
| Key Lock | `KEY_LOCK_ENABLE = yes` | `QK_LOCK`, then another key, "holds" that key indefinitely | Niche (simulated held Shift, held game key, etc.) |
| Unicode input | `UNICODE_ENABLE` / `UNICODEMAP_ENABLE` | Send arbitrary Unicode text (emoji, accented chars, arrows) from a keycode | Could dedicate a key to one frequently-typed special character |

## Lighting & feedback (superseded or not applicable)

- **RGB Light**, **Backlight**, **LED Matrix** — older/simpler lighting systems, superseded by the RGB Matrix already in use here; not upgrades.
- **Audio**, **Haptic Feedback** — need a piezo speaker / vibration motor respectively; almost certainly absent from this board's BOM.

## Hardware-dependent, not applicable here

<details>
<summary>Pointing Device, Digitizer, PS/2 Mouse, DIP Switch, Battery, Wireless/BLE, Joystick</summary>

All of these need physical hardware this keyboard doesn't have:

- **Pointing Device** — trackball/trackpad sensor (e.g. PMW3360) integration
- **Digitizer** — stylus/touch input
- **PS/2 Mouse** — legacy PS/2 trackpoint/trackball
- **DIP Switch** — physical DIP switches
- **Battery** — battery-level reporting for wireless boards
- **Wireless/BLE** — this board is wired USB only
- **Joystick** — HID gamepad axis/button output; technically possible without extra hardware but pointless on a typing keyboard

</details>

## Advanced / niche

| Feature | Requires | Description |
|---|---|---|
| Secure Mode | `SECURE_ENABLE = yes` | Lock/unlock a "safe" state that disables certain features unless unlocked; more relevant for shared/public-facing keyboards |
| Stenography | `STENO_ENABLE = yes` | Plover-compatible chorded steno input; only useful if you actually type steno-style |
| MIDI | `MIDI_ENABLE = yes` | Turns the keyboard into a MIDI controller |
| Sequencer | `SEQUENCER_ENABLE = yes` | MIDI step sequencer |
| RawHID | `RAW_ENABLE = yes` | Custom raw HID channel to companion host software — needs that companion app to exist |
| Programmable Button | `PROGRAMMABLE_BUTTON_ENABLE = yes` | Sends a fully custom signal for third-party remap software (VIA-style) to interpret; not useful without such software |
| Community Modules | — (module system) | QMK's plugin system for community-contributed features beyond core; an extensibility note more than a specific keycode |
| OS Detection | `OS_DETECTION_ENABLE = yes` | Detect host OS at runtime and branch firmware behavior; already flagged separately in `CLAUDE.md`'s sibling-keyboard survey as unported here |

## Candidates for the 3 dead keys

Short-list from the discussion that led to this doc — left unchecked since no decision has been made yet. Check off (or delete) once one is picked:

- [ ] Dynamic Macros (2–5 keys depending on slot/stop-key configuration)
- [x] ~~Leader Key (1 key, unlimited custom sequences)~~ — **considered and declined.** Claude suggested it unprompted (as a way to fold Undo/Redo into the freed corner keys without spending more physical keys on them) during the session that placed `[`/`]` on the pinky corners; the user pushed back ("I don't remember asking for that") and it was dropped. Not re-suggested unless asked.
- [ ] Layer Lock (1 key, locks whatever `MO()` layer is currently held)
- [ ] Repeat Key / Alternate Repeat Key (1–2 keys)
- [ ] Wire up the already-enabled Mouse Keys feature
- [ ] EEPROM Clear (1 key, resets persisted RGB/layer settings to defaults)

A later session's discussion converged on pairing the two remaining open
candidates into a trio that would fill all 3 dead keys at once: **Layer
Lock** on `[4,1]` (pairs naturally with the momentary-only Nav/Fn/Adjust
design — think Vim's modal editing, but for layers), **Repeat Key** on
`[4,2]` (mirrors Vim's `.` command, works host-wide), and **Alternate Repeat
Key** on `[9,1]` (auto-closes brackets/quotes via a
`get_alt_repeat_key_keycode_user()` table, e.g. after `(` sends `)`).
Presented to the user but not yet decided on — still open.

Also flagged but unrelated to the dead keys: the `_NAV`/`_FN` layers' two
encoders are currently `KC_TRNS` in both directions (do nothing beyond
falling through to `_BASE`'s Volume/Brightness). That's free real estate
needing no new physical keys — one idea raised was macOS Spaces switching
(`Ctrl+Left`/`Ctrl+Right`) on the `_NAV` layer's left encoder, since nothing
in this keymap currently switches virtual desktops. Not decided.

## Sources

- `docs/features/` — full feature documentation directory (49 files at time of writing)
- `docs/keycodes_magic.md` — Magic Keycodes reference
- `quantum/keycodes.h` — keycode range definitions (`QK_*` ranges and aliases)
- `quantum/process_keycode/process_grave_esc.c`, `process_magic.c`, `process_quantum.c` — implementation source for the "Individual special keycodes" section above, read directly to confirm exact behavior rather than paraphrasing docs
