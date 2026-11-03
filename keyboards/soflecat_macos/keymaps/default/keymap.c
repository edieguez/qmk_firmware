/* Copyright 2023 Brian Low
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include QMK_KEYBOARD_H

enum layers {
    _BASE,
    _LOWER,
    _UPPER,
    _ADJUST,
};

enum {
    // Caps Lock on tap, Caps Word toggle on double-tap.
    TD_CAPS = 0,
};

enum custom_keycodes {
    // Sends the real macOS Globe-key Consumer-page usage (AC Next Keyboard
    // Layout Select, 0x29D) via a raw HID consumer report. KC_LNG1 (Keyboard
    // LANG1, HID Keyboard-page 0x90) looks superficially similar but is a
    // Korean Hangul/English IME toggle, unrelated to macOS's physical Globe
    // key -- it never triggered the System Settings > Keyboard > "Press
    // Globe key to" behavior. QMK core has no keycode wired to 0x29D, so
    // it's sent manually below.
    MACOS_GLOBE = SAFE_RANGE,
    // Like OSM(mod) -- tap to arm a one-shot, hold for a plain modifier --
    // plus the "tap again to cancel" that OSM lacks: a second tap while
    // one-shot-armed just re-arms it there (add_oneshot_mods() is
    // idempotent), since a deliberate second tap comes too late to read as
    // the rapid double-tap ONESHOT_TAP_TOGGLE wants. See
    // process_oneshot_key() below.
    OS_SFT,
    OS_CMD,
};

// Press registers the real modifier straight away, so a hold chords like a
// normal Shift/Cmd (hold Shift + arrows, "QMK"). Release decides what the
// press was: a quick, uninterrupted press was a tap and arms the one-shot;
// anything else was a hold and just ends. A press while that mod is
// already one-shot-armed cancels it instead.
static uint8_t  os_held_mods       = 0;
static bool     os_held_interrupted = false;
static uint16_t os_pressed_at      = 0;

static void process_oneshot_key(uint8_t mod, keyrecord_t *record) {
    if (record->event.pressed) {
        if (get_oneshot_mods() & mod) {
            del_oneshot_mods(mod);
            return;
        }
        register_mods(mod);
        os_held_mods |= mod;
        os_held_interrupted = false;
        os_pressed_at       = timer_read();
    } else if (os_held_mods & mod) {
        unregister_mods(mod);
        os_held_mods &= ~mod;
        if (!os_held_interrupted && timer_elapsed(os_pressed_at) < TAPPING_TERM) {
            add_oneshot_mods(mod);
        }
    }
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (record->event.pressed && os_held_mods && keycode != OS_SFT && keycode != OS_CMD) {
        os_held_interrupted = true;
    }
    switch (keycode) {
        case MACOS_GLOBE:
            if (record->event.pressed) {
                host_consumer_send(AC_NEXT_KEYBOARD_LAYOUT_SELECT);
            } else {
                host_consumer_send(0);
            }
            return false;
        case OS_SFT:
            process_oneshot_key(MOD_BIT(KC_LSFT), record);
            return false;
        case OS_CMD:
            process_oneshot_key(MOD_BIT(KC_LGUI), record);
            return false;
        default:
            return true;
    }
}

// ACTION_TAP_DANCE_DOUBLE's second tap is sent via register_code16(), which
// only ever produces a raw HID report -- it bypasses process_record(), so it
// can't reach quantum keycodes like CW_TOGG whose behavior lives entirely in
// a process_record hook (process_caps_word.c). Caps Word's tap must instead
// call caps_word_toggle() directly, hence the "advanced" dance below instead
// of ACTION_TAP_DANCE_DOUBLE(CW_TOGG, KC_CAPS). Single tap -> Caps Word
// (the more frequently wanted of the two), double tap -> Caps Lock -- except
// a single tap while Caps Lock is already on turns Caps Lock back off
// instead, so there's a quick one-tap way out of it without needing another
// double tap or activating Caps Word by mistake.
//
// caps_dance_sent_capslock remembers which branch of caps_dance_finished()
// actually sent a KC_CAPS press, since caps_dance_reset() needs to know
// whether to send the matching release -- state->count alone can no longer
// tell (both a double tap and a single tap while Caps Lock is on hit the
// count==1 vs !=1 cases differently but both send KC_CAPS).
static bool caps_dance_sent_capslock = false;

static void caps_dance_finished(tap_dance_state_t *state, void *user_data) {
    if (state->count == 1) {
        if (host_keyboard_led_state().caps_lock) {
            register_code(KC_CAPS);
            caps_dance_sent_capslock = true;
        } else {
            caps_word_toggle();
            caps_dance_sent_capslock = false;
        }
    } else {
        register_code(KC_CAPS);
        caps_dance_sent_capslock = true;
    }
}

static void caps_dance_reset(tap_dance_state_t *state, void *user_data) {
    if (caps_dance_sent_capslock) {
        wait_ms(TAP_CODE_DELAY);
        unregister_code(KC_CAPS);
    }
}

tap_dance_action_t tap_dance_actions[] = {
    [TD_CAPS] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, caps_dance_finished, caps_dance_reset),
};

// The default caps_word_press_user() treats any key it doesn't recognize as
// word-breaking and cancels Caps Word immediately on keydown -- including
// this key's own raw TD(TD_CAPS) keycode. Tap dance resolution is
// asynchronous (tap_dance_task() decides "single tap" ~TAPPING_TERM later,
// not synchronously on keydown), so without this override, re-tapping to
// turn Caps Word off actually: (1) cancels it immediately via the default
// word-breaking check, then (2) ~200ms later, caps_dance_finished()'s own
// caps_word_toggle() call flips it back on, since by then it reads as off.
// Treating TD(TD_CAPS) as word-continuing here stops the hook from racing
// with the tap dance's own, deliberate toggle.
bool caps_word_press_user(uint16_t keycode) {
    switch (keycode) {
        case TD(TD_CAPS):
            return true;
        case KC_A ... KC_Z:
        case KC_MINS:
            add_weak_mods(MOD_BIT(KC_LSFT));
            return true;
        case KC_1 ... KC_0:
        case KC_BSPC:
        case KC_DEL:
        case KC_UNDS:
            return true;
        default:
            return false;
    }
}

// Flow Tap (FLOW_TAP_TERM in config.h) resolves a mod-tap key to an instant
// tap purely based on how soon it follows the previous keystroke -- before
// Chordal Hold gets a chance to see what key comes *next*. That defeats
// Chordal Hold's opposite-hand-forces-hold rule for the Shift/GUI home row
// mods: typing at a normal pace and then reaching for Shift+/ ('?'), a
// Shift+letter capital, or Cmd+V lets Flow Tap commit F/J or D/K to a tap
// first, producing e.g. "f/" instead of "?", or "kv" instead of Cmd+V.
// Disable Flow Tap on just these four keys so they fall back to Chordal
// Hold + Permissive Hold instead; every other tap-hold key (the Alt/Ctrl
// home row mods) keeps the default Flow Tap behavior.
uint16_t get_flow_tap_term(uint16_t keycode, keyrecord_t *record, uint16_t prev_keycode) {
    switch (keycode) {
        case LSFT_T(KC_F):
        case RSFT_T(KC_J):
        case LGUI_T(KC_D):
        case RGUI_T(KC_K):
            return 0;
        default:
            return is_flow_tap_key(keycode) && is_flow_tap_key(prev_keycode) ? FLOW_TAP_TERM : 0;
    }
}

// Complementary fix to the Flow Tap exclusion above, for the same "f/"/
// "kv" instead of "?"/Cmd+V symptom: Chordal Hold and Permissive Hold both
// need F/J/D/K to still be physically held when the opposite-hand key
// (e.g. "/" or "v") is pressed, to recognize a chord. At high typing speed
// there's sometimes close to zero overlap -- the mod-tap key gets released
// almost the instant the other key is pressed -- leaving no chord for
// either mechanism to catch, since by definition there's nothing to
// detect once the key's already up. Shift and GUI are disproportionately
// prone to this because they're often paired with a farther-reach key
// (top row or opposite corner), which naturally shortens overlap. A longer
// tapping term just for these four keys gives more margin for that overlap
// to still count. Requires TAPPING_TERM_PER_KEY in config.h to actually be
// consulted.
//
// +80ms wasn't enough margin in practice (still regularly missed chords,
// e.g. "f/" instead of "?"). HOLD_ON_OTHER_KEY_PRESS_PER_KEY was tried as
// a stronger fix (settling the hold on press rather than press+release),
// but overcorrected into firing on ordinary opposite-hand bigrams that
// were never meant as a chord ("fi" -> "I", "f/" -> "?" even when "/" was
// wanted) -- F/J/D/K are too common in plain English for that heuristic
// to hold up. +150ms is a middle ground: more margin than +80ms without
// Hold On Other Key Press's complete disregard for the interrupting key's
// release.
uint16_t get_tapping_term(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case LSFT_T(KC_F):
        case RSFT_T(KC_J):
        case LGUI_T(KC_D):
        case RGUI_T(KC_K):
            return TAPPING_TERM + 150;
        default:
            return TAPPING_TERM;
    }
}

// Permissive Hold settles a mod-tap as held whenever an opposite-hand key is
// pressed and released inside it -- exactly what a rolled cross-hand bigram
// looks like, so "also" became "a" + Alt+L + Ctrl+S ("aso"/"alo"). Keep it
// only for Shift/GUI (F/J/D/K), which need fast chords ("?", Cmd+V); the
// Alt/Ctrl keys (A/S/L/;) now only hold once held past the tapping term.
bool get_permissive_hold(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case LSFT_T(KC_F):
        case RSFT_T(KC_J):
        case LGUI_T(KC_D):
        case RGUI_T(KC_K):
            return true;
        default:
            return false;
    }
}

// Shift+Esc sends ~, replacing Grave Escape (QK_GESC), whose built-in
// GUI-held case is broken on macOS: Cmd+Esc there sends Cmd+` (grave),
// which macOS intercepts as "move focus to next window" instead of
// passing through -- so Cmd+Esc previously did nothing, including
// failing to cancel Cmd+Tab's app switcher. Plain KC_ESC plus this single
// override (QMK's own documented fix, see docs/features/key_overrides.md)
// fixes that: Cmd+Esc now falls through untouched as plain Escape with
// Cmd still held, while Shift+Esc keeps working as before.
const key_override_t tilde_esc_override = ko_make_basic(MOD_MASK_SHIFT, KC_ESC, S(KC_GRV));

// Option+letter overrides for symbols that are awkward to reach otherwise
// (no thumb/layer hold needed, and unlike Combos these only fire when
// Option is actually held, so there's no simultaneous-press timing window
// to misfire during normal typing). Option is already one reach away via
// the LALT_T(KC_A)/RALT_T(KC_SCLN) home row mods.
const key_override_t equal_key_override = ko_make_basic(MOD_MASK_ALT, KC_MINUS, KC_EQUAL);
const key_override_t grave_key_override = ko_make_basic(MOD_MASK_ALT, KC_QUOTE, KC_GRV);

const key_override_t *key_overrides[] = {
    &tilde_esc_override,
    &equal_key_override,
    &grave_key_override,
};

// Chordal Hold's default handedness table marks every thumb key (and knob
// click) as belonging to its half, so a home row mod plus a thumb key on
// the same side -- Cmd+Enter / Cmd+Bspc (K + right thumb), Cmd+Space /
// Shift+Space (D/F + Space), Shift+Enter (J + Enter) -- counts as a
// same-hand chord and settles as a tap unless held past the tapping term
// (350ms for D/F/J/K). '*' exempts the thumb row and the two knob clicks
// from the opposite-hands rule so those chords settle as held on the
// thumb key's press; letter-to-letter pairs keep their L/R handedness.
const char chordal_hold_layout[MATRIX_ROWS][MATRIX_COLS] PROGMEM = LAYOUT(
    'L', 'L', 'L', 'L', 'L', 'L',                     'R', 'R', 'R', 'R', 'R', 'R',
    'L', 'L', 'L', 'L', 'L', 'L',                     'R', 'R', 'R', 'R', 'R', 'R',
    'L', 'L', 'L', 'L', 'L', 'L',                     'R', 'R', 'R', 'R', 'R', 'R',
    'L', 'L', 'L', 'L', 'L', 'L', '*',           '*', 'R', 'R', 'R', 'R', 'R', 'R',
                   '*', '*', '*', '*', '*',   '*', '*', '*', '*', '*'
);

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
/*
 * BASE -- thumb clusters: Adjust|OS_Cmd|OS_Sft|Lower|Space on the left,
 * Enter|Bspc|Upper|[|] on the right. Lower and Adjust are left-thumb
 * holds so the right hand can stay on the mouse; the two One Shot Mods
 * (OSM) are on the left for the same reason -- they exist for
 * mouse-chorded actions like Cmd-click multiselect or Shift-click
 * range-select, so arming them must not pull the mouse hand back to the
 * keyboard. A single tap latches the modifier until the next *keyboard*
 * keypress clears it: QMK never sees mouse clicks as key events, so a
 * click neither resolves a mod-tap's hold/tap decision in time (which is
 * why LGUI_T(KC_D)/LSFT_T(KC_F) don't work for this) nor clears a one-shot
 * early. These are the custom OS_SFT/OS_CMD keycodes (see enum
 * custom_keycodes above), not plain OSM(): tapping again while armed
 * cancels instead of re-arming; holding still works as a plain modifier
 * like OSM() does. Upper (F1-F12) took OS_Cmd's old right
 * thumb slot in exchange; F1-F5 are then an opposite-hand reach. Globe
 * (rare: keyboard-layout switching) and = (less frequent than the
 * brackets) were judged cheapest to demote to the pinky corners.
 * ,-----------------------------------------------------.                    ,-----------------------------------------------------.
 * | Esc  |   1  |   2  |   3  |   4  |   5  |                                |   6  |   7  |   8  |   9  |   0  |   -  |
 * |------+------+------+------+------+------|                                |------+------+------+------+------+------|
 * | Tab  |   Q  |   W  |   E  |   R  |   T  |                                |   Y  |   U  |   I  |   O  |   P  |  \   |
 * |------+------+------+------+------+------|                                |------+------+------+------+------+------|
 * |CW/Caps|A/LAlt|S/LCtl|D/LGui|F/LSft|   G  |-------.                ,-------|   H  |J/RSft|K/RGui|L/RCtl|;/RAlt|  '   |
 * |------+------+------+------+------+------|  Mute |                | Play  |------+------+------+------+------+------|
 * |Globe  |   Z  |   X  |   C  |   V  |   B  |-------|                |-------|   N  |   M  |   ,  |   .  |   /  |  =   |
 * `-----------------------------------------/       /                \      \-----------------------------------------'
 *                       | Adj  |OS_Cmd|OS_Sft|LOWER | Space|    |Enter | Bspc |UPPER |  [   |  ]   |
 *                       `----------------------------------'              '------''---------------------------'
 */
    [_BASE] = LAYOUT(
        KC_ESC,      KC_1,          KC_2,          KC_3,          KC_4,          KC_5,                                          KC_6,          KC_7,          KC_8,          KC_9,          KC_0,            KC_MINUS,
        KC_TAB,      KC_Q,          KC_W,          KC_E,          KC_R,          KC_T,                                          KC_Y,          KC_U,          KC_I,          KC_O,          KC_P,            KC_BSLS,
        TD(TD_CAPS), LALT_T(KC_A),  LCTL_T(KC_S),  LGUI_T(KC_D),  LSFT_T(KC_F),  KC_G,                                          KC_H,          RSFT_T(KC_J),  RGUI_T(KC_K),  RCTL_T(KC_L),  RALT_T(KC_SCLN), KC_QUOTE,
        MACOS_GLOBE,   KC_Z,          KC_X,          KC_C,          KC_V,          KC_B,    KC_MUTE,             KC_MPLY,        KC_N,          KC_M,          KC_COMMA,      KC_DOT,        KC_SLASH,        KC_EQUAL,
                                                    MO(_ADJUST),   OS_CMD,       OS_SFT,    MO(_LOWER), KC_SPACE, KC_ENTER, KC_BSPC, MO(_UPPER), KC_LBRC,    KC_RBRC
    ),

/*
 * LOWER (formerly "NAV") -- arrows plus macOS-native word/line/document
 * navigation stacked around the arrow cluster: Option+Left/Right (word
 * jump) and Page Down/Up (one screen, cursor stays put) above the arrows
 * they scale up, and Option+Delete (word delete) next to it. macOS's own
 * Home/End equivalents -- Cmd+Left/Right (line start/end) and Cmd+Up/Down
 * (document start/end) -- live in the outer column, in that order,
 * standing in for the raw Home/End keycodes (which macOS handles
 * inconsistently). The thumb-cluster Backspace key sends
 * Option+Backspace here (word delete backwards), and the thumb-cluster
 * Enter key sends Shift+Enter (newline without submitting, for chat apps).
 * Delete lives at the N finger position (no dedicated thumb key for it --
 * all 10 thumb keys are spoken for). Grave/tilde (no dedicated thumb key
 * free for it either) lands on the Esc finger position: hold LOWER, tap
 * the top-left key for ` (Shift for ~). The remaining thumb keys (Adjust/
 * OS_Cmd/OS_Sft taps, and Upper/[/] on the right) just keep their _BASE
 * meaning here, transparent. Each overrides its _BASE meaning. Blank
 * cells are transparent (same key as _BASE).
 * ,-----------------------------------------------------.                    ,-----------------------------------------------------.
 * |   `  |      |      |      |      |      |                                |      |      |      |      |      |LnStrt|
 * |------+------+------+------+------+------|                                |------+------+------+------+------+------|
 * |      |      |      |      |      |      |                                | wLeft| PgDn | PgUp |wRight|      | LnEnd|
 * |------+------+------+------+------+------|                                |------+------+------+------+------+------|
 * |      |      |      |      |      |      |-------.                ,-------|  Left|  Down|   Up | Right|      |DocTop|
 * |------+------+------+------+------+------|       |                |       |------+------+------+------+------+------|
 * |      |      |      |      |      |      |-------|                |-------|  Del |      |      |      |      |DocEnd|
 * `-----------------------------------------/       /                \      \-----------------------------------------'
 *                       |      |      |      |LOWER |      |    |Sft+Enter| wBspc|      |      |      |
 *                       `----------------------------------'              '------''---------------------------'
 */
    [_LOWER] = LAYOUT(
        KC_GRV,   KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,                                     KC_TRNS,    KC_TRNS, KC_TRNS,   KC_TRNS,     KC_TRNS,    G(KC_LEFT),
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,                                     A(KC_LEFT), KC_PGDN, KC_PGUP,   A(KC_RIGHT), KC_TRNS,   G(KC_RIGHT),
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,                                     KC_LEFT,    KC_DOWN, KC_UP,     KC_RIGHT,    KC_TRNS,    G(KC_UP),
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,          KC_TRNS,           KC_DEL,     KC_TRNS,    KC_TRNS, KC_TRNS,     KC_TRNS,   G(KC_DOWN),
                                    KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, S(KC_ENTER),    A(KC_BSPC), KC_TRNS, KC_TRNS, KC_TRNS
    ),

/*
 * UPPER (formerly "FN") -- F1-F12 on the number row (F11/F12 on T/Y). Blank
 * cells are transparent (same key as _BASE).
 * ,-----------------------------------------------------.                    ,-----------------------------------------------------.
 * |      |  F1  |  F2  |  F3  |  F4  |  F5  |                                |  F6  |  F7  |  F8  |  F9  |  F10 |      |
 * |------+------+------+------+------+------|                                |------+------+------+------+------+------|
 * |      |      |      |      |      |  F11 |                                |  F12 |      |      |      |      |      |
 * |------+------+------+------+------+------|                                |------+------+------+------+------+------|
 * |      |      |      |      |      |      |-------.                ,-------|      |      |      |      |      |      |
 * |------+------+------+------+------+------|       |                |       |------+------+------+------+------+------|
 * |      |      |      |      |      |      |-------|                |-------|      |      |      |      |      |      |
 * `-----------------------------------------/       /                \      \-----------------------------------------'
 *                       |      |      |      |      |      |    |      |      | UPPER|      |      |
 *                       `----------------------------------'    '------''---------------------------'
 */
    [_UPPER] = LAYOUT(
        KC_TRNS,  KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,                                       KC_F6,   KC_F7,   KC_F8,    KC_F9,   KC_F10,  KC_TRNS,
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_F11,                                       KC_F12,  KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,                                     KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,          KC_TRNS,           KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS,
                                    KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    ),

/*
 * ADJUST -- held via the outermost left-thumb key. Row 1 is every RGB
 * matrix control, laid out so no -/+ pair straddles the two halves (the
 * Flag-/Flag+ keys were dropped: all 58 LEDs carry the same keylight flag,
 * so cycling flags was just a four-step version of RGBTog). Row 2 carries
 * the macOS F-row functions that have no knob position, on the same columns
 * their F-key counterparts occupy on _UPPER (W/E/R = F3/F4/F5): Mission
 * Control, Spotlight (Cmd+Space), Dictation. Dictation and Do Not Disturb
 * have no HID usage QMK can send (Apple uses a vendor page), so they're
 * chords that must be bound once in System Settings: Ctrl+Opt+Cmd+D for
 * Keyboard > Dictation > Shortcut, Ctrl+Opt+Cmd+N for Keyboard Shortcuts >
 * Mission Control > "Turn Do Not Disturb On/Off". The knobs here are media,
 * not RGB, and both on the right knob so they're a free-hand reach while
 * the left thumb holds Adjust: click = Do Not Disturb (F6), turn =
 * previous/next track (F7/F9). Left click stays transparent (Mute).
 * Blank cells are transparent (same key as _BASE).
 * ,-----------------------------------------------------.                    ,-----------------------------------------------------.
 * |RGBTog|Mode- |Mode+ | Hue- | Hue+ |      |                                | Sat- | Sat+ | Bri- | Bri+ | Spd- | Spd+ |
 * |------+------+------+------+------+------|                                |------+------+------+------+------+------|
 * |      |      |MCtrl |Spotl | Dict |      |                                |      |      |      |      |      |      |
 * |------+------+------+------+------+------|                                |------+------+------+------+------+------|
 * |      |      |      |      |      |      |-------.                ,-------|      |      |      |      |      |      |
 * |------+------+------+------+------+------|       |                |  DND  |------+------+------+------+------+------|
 * |      |      |      |      |      |      |-------|                |-------|      |      |      |      |      |      |
 * `-----------------------------------------/       /                \      \-----------------------------------------'
 *                       | Adj  |      |      |      |      |    |      |      |      |      |      |
 *                       `----------------------------------'              '------''---------------------------'
 */
    [_ADJUST] = LAYOUT(
        RM_TOGG,  RM_PREV, RM_NEXT, RM_HUED,   RM_HUEU,    KC_TRNS,                                     RM_SATD, RM_SATU, RM_VALD,  RM_VALU, RM_SPDD, RM_SPDU,
        KC_TRNS,  KC_TRNS, KC_MCTL, G(KC_SPC), LCAG(KC_D), KC_TRNS,                                     KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS,   KC_TRNS,    KC_TRNS,                                     KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS,   KC_TRNS,    KC_TRNS, KC_TRNS,          LCAG(KC_N),        KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS,
                                    KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    ),
};

#if defined(ENCODER_MAP_ENABLE)
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    // Knobs mirror the Magic Keyboard F-row: left = brightness (F1/F2, sent
    // as F14/F15, which macOS maps to brightness down/up), right = volume
    // (F11/F12) on _BASE; right = previous/next track (F7/F9) on _ADJUST.
    [_BASE]   = { ENCODER_CCW_CW(KC_F14, KC_F15),   ENCODER_CCW_CW(KC_VOLD, KC_VOLU) },
    [_LOWER]  = { ENCODER_CCW_CW(KC_TRNS, KC_TRNS), ENCODER_CCW_CW(KC_TRNS, KC_TRNS) },
    [_UPPER]  = { ENCODER_CCW_CW(KC_TRNS, KC_TRNS), ENCODER_CCW_CW(KC_TRNS, KC_TRNS) },
    [_ADJUST] = { ENCODER_CCW_CW(KC_TRNS, KC_TRNS), ENCODER_CCW_CW(KC_MPRV, KC_MNXT) },
};
#endif
