# Sofle cheat sheet

How to use the board day to day. Positions are matrix `[row,col]`: left
rows 0–4, right rows 5–9, column 0 is the outer edge.

## Thumbs (BASE)

| Left `4,0` | `4,1` | `4,2` | `4,3` | `4,4` | | Right `9,4` | `9,3` | `9,2` | `9,1` | `9,0` |
|---|---|---|---|---|---|---|---|---|---|---|
| Login 1 | Login 2 | Globe | Lower (hold) | Space | | Enter | Backspace / Upper (hold) | `=` | Screenshot | Adjust (hold) |

## Home row mods

A S D F = Alt Ctrl Cmd Shift, mirrored on J K L ; (; is Alt). Tap for the
letter, hold for the modifier. Shift and Cmd turn on fast when you press a key
on the other hand. Thumbs count as either hand, so Cmd+Space works with D.

## Caps

`2,0`: one tap Caps Word (capitals until the end of the word), two taps Caps
Lock. One tap also turns Caps Lock off.

## Login macros (`4,0`, `4,1`)

- **Type it fast:** tap the thumb.
- **Type it at the pace you recorded:** hold F or J, then tap the thumb.
- **Record:** hold Adjust, press the thumb (`REC` on the screen), type the
  login, press the thumb again (`SAVED` and the key count).
- Up to 32 keys per slot. Capitals and symbols are kept. Survives power-off.
  `EMPTY` means nothing is recorded yet.

## Screenshots (`9,1`)

| Gesture | Shortcut | Takes |
|---|---|---|
| Tap | Cmd+Shift+4 | an area you drag (Space for a window) |
| Double tap | Cmd+Shift+3 | the whole screen |
| Hold | Cmd+Shift+5 | the capture panel, also for recording |

## Lower (hold `4,3`)

- **Arrows** on H J K L; **word jumps** on Y and O; right knob jumps by word.
- **Home, End, PgUp, PgDn** down the right outer column.
- **Delete** on Esc. **Option+Backspace** (delete word) on Backspace.
  **Shift+Enter** on Enter.
- **ñ** on N, **ü** on U, **¿** on /, **¡** on 1.

## Upper (hold Backspace `9,3`)

F1–F5 on the number row, F6–F10 on Q–T, F11/F12 on D/F. Left knob zooms
(Cmd+− / Cmd+=), its click resets the zoom.

## Adjust (hold `9,0`)

- **RGB:** toggle on Esc, mode −/+ on 1/2, hue −/+ on 3/4. Right number row:
  saturation, brightness, speed (− then +).
- **Mission Control** on W, **Dictation** on R, **Do Not Disturb** on the
  left knob click. Dictation and Do Not Disturb are Ctrl+Option+Cmd+D/N and
  must be bound in System Settings.
- **Right knob:** previous/next track.
- **Record logins:** Adjust + `4,0` / `4,1`, see above.

### Tuning the home row mods

If a letter comes out as a modifier, or a modifier as a letter, change that
key's timing:

1. Hold Adjust and tap A, S, D or F to pick Alt, Ctrl, Cmd or Shift. The
   screen shows its current time.
2. Turn the left knob: 5 ms a detent, 100–500 ms. Longer means harder to
   trigger the modifier by accident.
3. G shows all four times. B resets them (Alt/Ctrl 200, Cmd/Shift 350).

Saved automatically and kept after power-off. Each setting covers both
hands (F and J share Shift).

## Other keys

- **`~`** is Shift+Esc. **`` ` ``** is Option+'.
- **Globe** (`4,2`): the macOS Globe/Fn key, for the emoji picker and Fn
  shortcuts.

## Knobs

| | Left | Right |
|---|---|---|
| BASE | screen brightness, click Mute | volume, click Play/Pause |
| Lower | — | word jumps |
| Upper | zoom, click resets | — |
| Adjust | home row mod timing, click Do Not Disturb | previous/next track |

## Screens

The left (main) screen shows the layer, active modifiers, Caps Word / Caps
Lock and the last keys typed; messages (RGB changes, timings, logins) show
there for 1.5 s. The right screen shows the bongo cat and your typing speed.
