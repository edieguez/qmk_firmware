# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

This is a personal fork (`edieguez/qmk_firmware`, remote `origin`) of [QMK Firmware](https://github.com/qmk/qmk_firmware) (remote `upstream`), a C firmware for mechanical keyboards targeting AVR and ARM (ChibiOS) microcontrollers. The active work in this checkout is the user's two personal keyboards, `keyboards/soflecat/` (Sofle Choc, RP2040, macOS) and `keyboards/lily58cat/` (stock Lily58, ATmega32u4, Linux and macOS keymaps). It's not upstream QMK feature development; see "Handoff" at the end. `keyboards/soflecat/CLAUDE.md` has detailed, load-bearing notes specific to that board (why the folder was renamed, a past silent-build-failure bug, flashing procedure); read it before touching that directory.

The `qmk` CLI is installed in a virtualenv (`qmk doctor` reports CLI 1.1.8) — prefer it over raw `make` where a `qmk` subcommand exists.

## Build & compile commands

Run from the repo root.

```bash
# Compile a specific keyboard+keymap
qmk compile -kb <keyboard> -km <keymap>
make <keyboard>:<keymap>              # equivalent make form, colons/slashes in keyboard name become underscores in the target

# Compile with an environment override (e.g. converting a keyboard.json's pinout to a different MCU)
qmk compile -kb <keyboard> -km <keymap> -e CONVERT_TO=<target>

# Flash (only meaningful when a programmer/DFU is wired up — many boards flash via UF2 drag-and-drop instead)
make <keyboard>:<keymap>:flash

# Scaffold a new keymap from the keyboard's default
qmk new-keymap -kb <keyboard>

# List all known keyboards / show info for one
qmk list-keyboards
qmk info -kb <keyboard>
```

Full compiles (`make all`, `make <keyboard>:all`) build every keymap for every keyboard and are extremely slow — avoid unless specifically asked to verify broad compatibility. When iterating on one keyboard/keymap, always scope the build to it.

## Linting & formatting

```bash
# Validate a keyboard's info.json/keyboard.json and layout data (what CI runs on keyboard PRs)
qmk lint --strict --keyboard <keyboard>
qmk info -l --keyboard <keyboard>

# Auto-format changed files (what CI runs for core/driver/platform PRs)
qmk format-c --core-only <file...>     # clang-format, core C/H files
qmk format-python <file...>            # yapf
qmk format-text <file...>              # misc text formatting (json, md, mk...)

# CI-only integrity checks — rarely needed locally
qmk ci-validate-keyboard-targets
qmk ci-validate-aliases
```

`clang-format` config is at `.clang-format` (Google-based, 4-space indent, 1000-col limit); pass `-style=file` if invoking `clang-format` directly instead of via `qmk format-c`. Some files intentionally wrap sensitive macros (e.g. `LAYOUT(...)`) in `// clang-format off` / `// clang-format on` — don't strip those guards.

## Tests

```bash
# C unit tests (Google Test/Mock, run via qmk's wrapper — what CI runs)
qmk test-c -j $(nproc)

# Equivalent make forms
make test:all                          # every C unit test
make test:<substring>                  # filter by name, e.g. make test:tap_hold_configurations
make test:<feature>:<substring>        # e.g. make test:retro_shift:tap_hold_configurations
make test:all DEBUG=1                  # forward debug console output to stderr

# Python unit tests (CLI/lib/python) — nose2, config in nose2.cfg (start-dir lib/python/qmk/tests)
python3 -m nose2
```

C unit tests live under `tests/<feature>/` (see `tests/` for the current list: `basic`, `combo`, `tap_dance`, `caps_word`, etc.), each with its own `rules.mk`/`testlist.mk`, and are compiled with the host's native compiler (not cross-compiled for a keyboard) — failures can be debugged directly with gdb against the binaries in `build/test/`.

## Architecture

QMK is layered from generic to specific; a given symbol may be defined/overridden at any of these levels, with more specific levels winning:

- **`tmk_core/`** — the historical core (`protocol/`, USB HID handling via LUFA/ChibiOS-USB).
- **`platforms/`** — MCU-family glue: `avr/`, `chibios/` (ARM), and `platforms/test/` (host-native, used for unit tests). Defines low-level GPIO/timer/EEPROM/bootloader primitives that everything above builds on.
- **`quantum/`** — the shared feature layer nearly all keyboards use: keymap/layer engine (`action*.c`), `process_keycode/` (one file per feature: tap dance, combos, leader key, unicode, etc.), `rgb_matrix/`, `rgblight/`, `encoder/`, `split_common/` (split-keyboard transport), `painter/` (Quantum Painter display driver), `sequencer/`.
- **`drivers/`** — peripheral chip drivers (OLED controllers, LED drivers, sensors) consumed by `quantum/` features.
- **`keyboards/<vendor>/<board>/`** — one keyboard: `keyboard.json` (data-driven config: MCU, matrix pins, layouts, RGB matrix positions — validated against `data/schemas/keyboard.jsonschema`), a keyboard-level `.c`/`.h` for board-specific hooks (`*_kb()` functions), and `keymaps/<name>/` subfolders each with `keymap.c` (the `keymaps[][MATRIX_ROWS][MATRIX_COLS]` array via the board's `LAYOUT()` macro), optional `config.h`, `rules.mk` (per-keymap feature flags), and optional `encoder_map[]` when `ENCODER_MAP_ENABLE` is set.
- **`layouts/`** — community layout definitions (e.g. `ortho_4x12`) shareable across keyboards that implement the same physical layout.
- **`users/`** — legacy userspace dirs (per repo policy, new user-keymap/userspace contributions to upstream are no longer accepted; this only matters for old boards).
- **`data/`** — the data-driven build system's source of truth: JSON Schemas (`data/schemas/`) that `keyboard.json`/`info.json`/keymap JSON are validated against, plus `constants/`, `mappings/` (keycode aliases), and `templates/` used by `qmk new-keyboard`/`qmk new-keymap`.
- **`builddefs/`** — the make build system itself (`build_keyboard.mk`, `build_test.mk`, etc.) — this is what turns `keyboard.json` + `keymap.c` into a `.hex`/`.uf2`/`.bin`. Keyboard-level `.c` auto-inclusion is name-matched here (see `keyboards/soflecat/CLAUDE.md` for a concrete gotcha this caused).
- **`lib/python/qmk/`** — the `qmk` CLI itself (Milc-based); `lib/python/qmk/tests` is what `nose2` runs against.
- **`tests/`** — C unit tests for `quantum/` features (see Tests section above).
- **`docs/`** — VitePress-powered docs source for docs.qmk.fm; preview locally with `qmk docs -b`.

Most stray `.hex`/`.bin`/`.uf2` files at the repo root are local build artifacts from `qmk compile`/`make`, not tracked/meaningful source.

## Coding conventions (C)

- 4-space soft-tab indent, opening brace at end of line, closing brace aligned with the block's opening statement; always use optional braces (`if (x) { return; }`, never brace-less).
- `#pragma once` for header guards, not `#ifndef`/`#define`.
- Line length is generally unbounded; if wrapping, prefer not wider than 76 columns.
- Match the style of surrounding code over these rules when the two conflict in existing files.

## Handoff (state as of 2026-11-17)

### Branches
- **`master` is the only branch that matters.** It holds both boards in one straight line on top of upstream `master` (`3329239097`): the Sofle commits, the `#26481` cherry-pick, the Lily58 commits, then the notes. It's pushed; `origin/master` matches.
- **`soflecat`** (recreated from `master` on 2026-11-17) is the working branch for the Sofle changes. The old remote branches (`origin/soflecat`, `origin/lily58`) are stale leftovers the user is deleting; don't compare against them or report their divergence.
- All three `CLAUDE.md` files and the soflecat notes are tracked and committed. Commit note changes like any other change.

### How the user works (follow strictly)
- **One step at a time:** implement, compile, report the flash size, hand over the flash command, wait for the user's test result, then commit. Don't start the next step unprompted.
- **Commit messages:** one plain subject line in the soflecat style (several changes joined with `;`), no body, **no Claude co-author/attribution line**.
- **Only change the user's board folders** (`keyboards/lily58cat/`, `keyboards/soflecat/`). Never edit upstream QMK files (`quantum/`, `platforms/`, `builddefs/`…), even for a real QMK bug: put the workaround in the board (keyboard `.c`, board `rules.mk` with `SRC`/`OPT_DEFS`, new files). Upstream's own commits cherry-picked from `develop` are fine. Branches stay based on upstream `master`.
- **No leftover git worktrees or stashes.** A worktree blocks its branch from being checked out. Stashes have been popped onto the wrong branch here. Park untested work on a clearly named WIP branch; branches can be built without checkout (temporary `GIT_INDEX_FILE` + `commit-tree`).
- **The user flashes the boards themselves.** Hand over the command; never copy onto a bootloader drive. Ask before any force push.
- **History is curated:** logical commits, no dead ends (rewritten once on request). Fold fixes into the commit they belong to while it's unpushed, and ask first.
- **Keys are named by matrix `[row,col]`** from `keyboard.json`: left rows 0–4, right rows 5–9, column 0 is the outer edge on both halves, row 4/9 = inner key at column 5 and thumbs at columns 4 → 1 (column 4 is the big resting key; the Sofle also has column 0). Both board `CLAUDE.md` files carry the diagram.

### Technical traps
- **Keyboard-level `.c` must match the folder name** (`keyboards/<kb>/<kb>.c`), or its hooks (OLEDs, RGB keys) silently disappear with no build error.
- **Lily58 flash is 97–100% full and the number depends on the compiler.** Measured 2026-11-15 with the user's usual toolchain: Linux 27,914 B, macOS 28,184 B of 28,672. Measured 2026-11-17 on this Mac with Homebrew `avr-gcc@8` 8.5.0: Linux 28,562 B (110 free), macOS 28,834 B (162 over, build fails). Measure every change against the same compiler. No `snprintf` and no floats on AVR. OS auto-detection won't fit.
- **AVR toolchain on this Mac:** `arm-none-eabi-gcc` is on `PATH` (Sofle builds work). Homebrew's `avr-gcc@8` is installed but keg-only, so `qmk compile -kb lily58cat` fails with `gccversion] Error 127` and `qmk doctor` says "Can't find avr-gcc" until you run `export PATH="/opt/homebrew/opt/avr-gcc@8/bin:/opt/homebrew/opt/avr-binutils/bin:$PATH"`. Details in `keyboards/lily58cat/CLAUDE.md`.
- **Stale build cache after moving files:** delete `.build/obj_<kb>_<keymap>/` if make complains about an old path. Always confirm the compile succeeded before trusting a `cmp` of `.hex` files; a failed build leaves the old file in place.
- **lily58cat has two keymaps from one source:** `keymaps/macos/keymap.c` defines `LILY58CAT_MACOS` and includes `../default/keymap.c`. Every OS-specific key lives in the one `#ifdef LILY58CAT_MACOS` block at the top of the default keymap. Shared config, rules and `lib/` live at the keyboard level.
- **Board details:** hardware findings and QMK workarounds (6+6 LED strips, US International AltGr and dead keys, the rgblight speed quirk, boot colour sync, the AVR split lock) are in `keyboards/lily58cat/CLAUDE.md`. Sofle details (layout, tap-hold tuning, OLED/RGB code, stuck-suspend recovery) are in `keyboards/soflecat/CLAUDE.md`.
- **Editor errors:** clangd shows bogus errors in board files because it doesn't know QMK's include paths. Trust `qmk compile`.
- **Something else in the user's setup has run `git stash` operations** (editor or another tool). If stashes or conflicts appear unexpectedly, inspect before acting.

### Open work
1. **Lily58 macOS keymap** (`-km macos`): not yet tested on a Mac. Dictation and Do Not Disturb need binding in System Settings (Ctrl+Option+Cmd+D/N).
2. **Sofle ñ/ü/¿/¡ on Lower, Delete on Lower+M** (`0bc5b94b40`): committed and pushed; a `soflecat_default.uf2` from 2026-11-16 sits in the repo root. Confirm it was flashed and tested.
3. **Sofle backlog:** `keyboards/soflecat/lessons-from-lily58cat.md`.
   - Items 1, 2 and 4 to port: Adjust on a thumb, PgUp/PgDn on the Lower outer column, a screenshot key. Item 3 (ñ) is done.
   - Items 5 and 8 to discuss: prune the 42 RGB effects, reassign the freed thumbs. Items 6 and 7 (notes refresh, `[row,col]` diagram) are done.
   - Item 9, roadmap: one OS-adaptive Sofle layout (Linux on a PC, macOS on a Mac).
4. **Drop the `#26481` cherry-pick** once upstream `master` contains it (it didn't as of the last `git fetch upstream`). A rebase drops it automatically.
