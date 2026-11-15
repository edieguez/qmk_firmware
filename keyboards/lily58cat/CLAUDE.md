# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

`keyboards/lily58cat/` is a personal QMK keyboard for the user's physically owned **stock Lily58** (rev1 PCB, ATmega32u4 Pro Micro with the Caterina bootloader, 6+6 underglow LEDs, two 128x32 OLEDs, no encoders). It is a port of `keyboards/soflecat/` (the user's Sofle on macOS), not a copy: macOS shortcuts were translated or dropped, and several things were redesigned after testing on the real board. `keyboard.json` started from `keyboards/lily58/rev1/`; its `keyboard_name`/`manufacturer`/`maintainer` ("Lily58"/"liliums") stay upstream's on purpose. Don't attribute the hardware to the user.

**The keyboard-level source file name is load-bearing.** `builddefs/build_keyboard.mk` only compiles a keyboard-level `.c` whose name matches the folder, and gives no error when it doesn't. Without `lily58cat.c`, the OLED status screen, the bongocat, the keylog and the RGB effect keys would silently stop working. If the folder is ever renamed, rename the file in the same commit. See `keyboards/soflecat/CLAUDE.md` for the time this bit the Sofle.

## Build & flash

Two keymaps from one source. `default` is the Linux layout (host runs US International). `macos` is the same layout with the Mac keys swapped in: `keymaps/macos/keymap.c` only defines `LILY58CAT_MACOS` and includes `../default/keymap.c`; every OS-specific key is a macro in the single `#ifdef LILY58CAT_MACOS` block at the top of the default keymap (home row Ctrl/Cmd, word and line jumps, Super vs Globe, brightness keys, ñ ü ¿ ¡, backtick, the Mac-only Adjust keys). The layer diagrams show the Linux keys. `config.h`, `rules.mk`, `lily58cat.c` and `lib/` are shared at the keyboard level; each keymap's `rules.mk` only adds `TAP_DANCE_ENABLE`.

```bash
qmk compile -kb lily58cat -km default   # Linux
qmk compile -kb lily58cat -km macos     # macOS, untested on a Mac so far
qmk flash   -kb lily58cat -km default   # press reset on the half when avrdude waits; repeat for the other half
```

No `CONVERT_TO`: this is a real AVR Pro Micro. The user flashes the board themselves. Compile and hand over the command, then wait for their test result. Bootmagic: hold `0,0` (Esc) while plugging in the left half, or `5,0` (`-`) for the right half. There's no `QK_BOOT` key on purpose.

## Flash budget (28,672 B, 97–100% used depending on the compiler)

Every feature has to earn its bytes. Measure before and after each change. `qmk compile` prints the size, and `avr-size .build/lily58cat_default.elf` gives RAM (data+bss, about 1.3 KB of 2.5 KB; the OLED buffer is 512 B of that). Only a few hundred bytes are left, so any new feature has to replace something.

**The size depends on which avr-gcc builds it.** Measured on 2026-11-15 (the toolchain the user normally builds with): Linux 27,914 B, macOS 28,184 B. Measured on 2026-11-17 on the Mac with Homebrew `avr-gcc@8` 8.5.0 (keg-only; it has to be put on `PATH` with `export PATH="/opt/homebrew/opt/avr-gcc@8/bin:/opt/homebrew/opt/avr-binutils/bin:$PATH"`): Linux 28,562 B (110 free), **macOS 28,834 B, 162 B over, the build fails `check-size`**. Compare sizes only between builds from the same compiler, and don't flash a Lily58 hex from a build that failed the size check.

Approximate costs measured on this build:

| Feature | Flash |
|---|---|
| Home row mod tuning (Chordal/Permissive/Flow Tap/Speculative Hold) | ~2.2 KB |
| Tap dance + Caps Word | ~1.5 KB |
| rgblight | ~2.9 KB |
| OLED status screen + keylog + RGB messages | ~5.5 KB |
| Lite bongocat (3 frames) + WPM, slave OLED | ~2.1 KB net of the logo it replaced |
| Lily58 logo bitmap (removed; going back frees ~1.5 KB) | ~0.5 KB |
| Breathing + rainbow mood + rainbow swirl | ~0.8 KB |
| Key Overrides (removed) | ~1.4 KB |
| Bongocat, full Sofle version (8 frames) | ~5.2 KB, doesn't fit |

`snprintf` is avoided on purpose, because on AVR it pulls in about 1.5 KB of vfprintf. Use `get_u8_str()`. Floats are avoided too: the bongocat timing uses integer maths so the soft-float library stays out.

## Hardware and host facts learned on the board

- **Underglow:** a strip of 6 WS2812 per half, each on its own half's D3. It isn't chained through the TRRS cable. Config: `led_count: 12`, `split_count: [6, 6]`. On this checkout that needs upstream's rgblight fix #26481 (`Fix out-of-range LED writes on split keyboards in rgblight`, cherry-picked from `develop`). Without it the right half writes past its LED buffer and the halves misbehave. When rebasing onto an upstream `master` that already contains the fix, drop the cherry-pick.
- **Upstream files stay untouched; workarounds for QMK bugs live in this folder.** The only exception is the #26481 cherry-pick above, which is upstream's own commit.
  - **Boot colour sync** (`housekeeping_task_kb` in `lily58cat.c`): the USB half is USB-suspended at power-up, so its first sync says "off". The other half drops colour updates while off, and turning on only syncs the mode, so it woke with its own stale colour. Fix: on every off-to-on change, the USB half also flags the colour as changed. It reads QMK's internal `rgblight_status`, so a rename upstream would break the build.
  - **AVR shared-memory lock** (`split_lock.c`, enabled from `rules.mk` with `PLATFORM_SUPPORTS_SYNCHRONIZATION`): on AVR `split_shared_memory_lock()` is a no-op, so a sync arriving between a slave handler's copy and its flag clear was lost.
- **rgblight speed:** the speed setting does nothing for breathing/mood/swirl. Each effect is several modes that differ only in speed (and in direction for swirl). `lily58cat.c` takes over `UG_NEXT/UG_PREV` (jump between effects) and `UG_SPDU/UG_SPDD` (step through that effect's variants) in `process_record_kb`.
- **Host layout is US International** (`setxkbmap`: `us`, variant `intl`):
  - **Right Alt is AltGr**, so the `;` home row mod sends **Left Alt** (`HM_SCLN`). Otherwise RAlt+Tab is a plain Tab.
  - **`` ` `` and `~` are dead keys**, so Shift+Esc, Alt+`'` and Lower+Esc send the symbol followed by a space (`tap_dead_key()` in `keymap.c`). Key Overrides can't send two keys, which is why they were dropped.
  - **ñ/¿/¡** come from dedicated `RALT(...)` keys on Lower, because there's no AltGr key. `ES_NTIL` is listed in `caps_word_press_user()` so Caps Word gives `Ñ`.
  - `'` is still a dead key on BASE, so á é í ó ú work natively.
- **Second OLED:** a lite bongocat (`lib/bongocat.c`): the Sofle's first idle frame shown still, plus its two typing frames, alternating faster with WPM. The WPM count comes over the split link (`SPLIT_WPM_ENABLE`). It replaced the Lily58 logo; the font's 0x80+ glyphs hold the status-screen icons (from splitkb/aurora/sofle_v2), not the Lily logo, so the logo would have to be a raw bitmap again.

## Editing the layout

- **Key references use matrix `[row,col]`** from `keyboard.json`. Left rows are 0–4, right rows 5–9, and column 0 is the outer edge on both halves. Row 4/9 holds the inner key (`col 5`) and the thumbs (`col 4` = big resting key, out to `col 1`; the Lily has no `4,0`/`9,0`). The user reviews and requests changes in this notation.

  ```
  │ 0,0 │ 0,1 │ 0,2 │ 0,3 │ 0,4 │ 0,5 │             │ 5,5 │ 5,4 │ 5,3 │ 5,2 │ 5,1 │ 5,0 │
  │ 1,0 │ 1,1 │ 1,2 │ 1,3 │ 1,4 │ 1,5 │             │ 6,5 │ 6,4 │ 6,3 │ 6,2 │ 6,1 │ 6,0 │
  │ 2,0 │ 2,1 │ 2,2 │ 2,3 │ 2,4 │ 2,5 │             │ 7,5 │ 7,4 │ 7,3 │ 7,2 │ 7,1 │ 7,0 │
  │ 3,0 │ 3,1 │ 3,2 │ 3,3 │ 3,4 │ 3,5 │ 4,5 │ │ 9,5 │ 8,5 │ 8,4 │ 8,3 │ 8,2 │ 8,1 │ 8,0 │
                    │ 4,1 │ 4,2 │ 4,3 │ 4,4 │ │ 9,4 │ 9,3 │ 9,2 │ 9,1 │
  BASE today:       │PrtSc│ Adj │Lower│Space│ │Enter│Bs/Up│  =  │Super│   (Super = Globe on macOS)
  ```
- Keep the ASCII-art comment above each layer in `keymap.c` in sync with the `LAYOUT(...)`.
- `chordal_hold_layout` marks every thumb and inner key `'*'`. Keep it that way, or same-side chords like Ctrl+Space settle as taps.
- Per-key tap-hold exceptions (`get_tapping_term`, `get_permissive_hold`, `get_flow_tap_term`, `get_speculative_hold`) apply to Shift and Ctrl, Cmd on macOS (`HM_F`, `HM_J`, `HM_D`, `HM_K`).
- Layers: Lower (`4,3`), Adjust (`4,2`) and Upper (`9,3`, Backspace on tap). No layer colours on the underglow: a strip can't show per-key hints, so they were removed.
