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
    // Plain OSM(mod) has no "tap again to cancel" behavior -- a second tap
    // while already one-shot-armed just re-arms it (add_oneshot_mods() is
    // idempotent), since by the time of a deliberate second tap enough real
    // time has usually passed that it reads as a fresh, independent press
    // rather than a rapid double-tap for ONESHOT_TAP_TOGGLE to catch. These
    // two custom keycodes replace OSM(MOD_LSFT)/OSM(MOD_LGUI) on _BASE to
    // add that explicitly: tap once to arm, tap again (any time before the
    // next real keypress consumes it) to cancel.
    OS_SFT,
    OS_CMD,
};

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case MACOS_GLOBE:
            if (record->event.pressed) {
                host_consumer_send(AC_NEXT_KEYBOARD_LAYOUT_SELECT);
            } else {
                host_consumer_send(0);
            }
            return false;
        case OS_SFT:
            if (record->event.pressed) {
                if (get_oneshot_mods() & MOD_BIT(KC_LSFT)) {
                    del_oneshot_mods(MOD_BIT(KC_LSFT));
                } else {
                    add_oneshot_mods(MOD_BIT(KC_LSFT));
                }
            }
            return false;
        case OS_CMD:
            if (record->event.pressed) {
                if (get_oneshot_mods() & MOD_BIT(KC_LGUI)) {
                    del_oneshot_mods(MOD_BIT(KC_LGUI));
                } else {
                    add_oneshot_mods(MOD_BIT(KC_LGUI));
                }
            }
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
uint16_t get_tapping_term(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case LSFT_T(KC_F):
        case RSFT_T(KC_J):
        case LGUI_T(KC_D):
        case RGUI_T(KC_K):
            return TAPPING_TERM + 80;
        default:
            return TAPPING_TERM;
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

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
/*
 * BASE -- the Globe thumb key is MACOS_GLOBE, a custom keycode (see above),
 * not a plain keycode. _ADJUST and _UPPER sit on the left thumb cluster
 * next to _LOWER (Globe|Adjust|Upper|Lower|Space), so either layer -- or
 * Adjust -- is reachable one-handed with the left thumb, since the right
 * hand is usually on the mouse. [ and ] moved off the finger-row corners
 * onto the right thumb cluster instead, and the vacated corners became One
 * Shot Mods (OSM) for Shift (left corner) and Cmd (right corner) -- not
 * home row mods, and not a plain hold either, for mouse-chorded actions
 * like Cmd-click multiselect or Shift-click range-select: a single tap
 * latches the modifier down and keeps it there (no need to keep a finger
 * on the key through a whole multi-click selection) until the next
 * *keyboard* keypress clears it. That's also why One Shot beats a plain
 * hold or the LGUI_T(KC_D)/LSFT_T(KC_F) home row mods here -- QMK never
 * sees mouse clicks as key events at all, so they neither resolve a
 * mod-tap's hold/tap decision in time nor clear a one-shot mod early; the
 * modifier just stays live across as many clicks as needed. OSM Cmd
 * landing on the right (mouse) hand's corner is a deliberate, accepted
 * tradeoff: Cmd-click needs a brief mouse-hand detour to tap it, unlike
 * Shift-click, which stays entirely on the left hand. These are the custom
 * OS_SFT/OS_CMD keycodes (see enum custom_keycodes above), not plain
 * OSM() -- tapping again while already armed cancels it instead of just
 * re-arming it, which is what plain OSM() would do.
 * ,-----------------------------------------------------.                    ,-----------------------------------------------------.
 * | Esc  |   1  |   2  |   3  |   4  |   5  |                                |   6  |   7  |   8  |   9  |   0  |   -  |
 * |------+------+------+------+------+------|                                |------+------+------+------+------+------|
 * | Tab  |   Q  |   W  |   E  |   R  |   T  |                                |   Y  |   U  |   I  |   O  |   P  |  \   |
 * |------+------+------+------+------+------|                                |------+------+------+------+------+------|
 * |CW/Caps|A/LAlt|S/LCtl|D/LGui|F/LSft|   G  |-------.                ,-------|   H  |J/RSft|K/RGui|L/RCtl|;/RAlt|  '   |
 * |------+------+------+------+------+------|  Mute |                | Play  |------+------+------+------+------+------|
 * |OS_Sft |   Z  |   X  |   C  |   V  |   B  |-------|                |-------|   N  |   M  |   ,  |   .  |   /  |OS_Cmd|
 * `-----------------------------------------/       /                \      \-----------------------------------------'
 *                       | Globe| Adj  | UPPER|LOWER | Space|    |Enter | Bspc |  [   |  ]   |  =   |
 *                       `----------------------------------'              '------''---------------------------'
 */
    [_BASE] = LAYOUT(
        KC_ESC,      KC_1,          KC_2,          KC_3,          KC_4,          KC_5,                                          KC_6,          KC_7,          KC_8,          KC_9,          KC_0,            KC_MINUS,
        KC_TAB,      KC_Q,          KC_W,          KC_E,          KC_R,          KC_T,                                          KC_Y,          KC_U,          KC_I,          KC_O,          KC_P,            KC_BSLS,
        TD(TD_CAPS), LALT_T(KC_A),  LCTL_T(KC_S),  LGUI_T(KC_D),  LSFT_T(KC_F),  KC_G,                                          KC_H,          RSFT_T(KC_J),  RGUI_T(KC_K),  RCTL_T(KC_L),  RALT_T(KC_SCLN), KC_QUOTE,
        OS_SFT,        KC_Z,          KC_X,          KC_C,          KC_V,          KC_B,    KC_MUTE,             KC_MPLY,        KC_N,          KC_M,          KC_COMMA,      KC_DOT,        KC_SLASH,        OS_CMD,
                                                    MACOS_GLOBE,   MO(_ADJUST),   MO(_UPPER),    MO(_LOWER), KC_SPACE, KC_ENTER, KC_BSPC, KC_LBRC, KC_RBRC,    KC_EQUAL
    ),

/*
 * LOWER (formerly "NAV") -- arrows plus macOS-native word/line/document
 * navigation stacked around the arrow cluster: Option+Left/Right (word
 * jump) above, and Option+Delete (word delete) next to it. macOS's own
 * Home/End/PgUp/PgDn equivalents -- Cmd+Left/Right (line start/end) and
 * Cmd+Up/Down (document start/end) -- live in the outer column, in that
 * order, standing in for the raw Home/End/PgUp/PgDn keycodes (which macOS
 * handles inconsistently). The thumb-cluster Backspace key sends
 * Option+Backspace here (word delete backwards), and the thumb-cluster
 * Enter key sends Shift+Enter (newline without submitting, for chat apps).
 * Delete lives at the N finger position (no dedicated thumb key for it --
 * all 10 thumb keys are spoken for). Grave/tilde (no dedicated thumb key
 * free for it either) lands on the Esc finger position: hold LOWER, tap
 * the top-left key for ` (Shift for ~). Equals/plus has its own dedicated
 * _BASE thumb key now (see above), so that position is transparent here.
 * Each overrides its _BASE meaning. Blank cells are transparent (same key
 * as _BASE).
 * ,-----------------------------------------------------.                    ,-----------------------------------------------------.
 * |   `  |      |      |      |      |      |                                |      |      |      |      |      |LnStrt|
 * |------+------+------+------+------+------|                                |------+------+------+------+------+------|
 * |      |      |      |      |      |      |                                | wLeft|      |      |wRight|      | LnEnd|
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
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,                                     A(KC_LEFT), KC_TRNS, KC_TRNS,   A(KC_RIGHT), KC_TRNS,   G(KC_RIGHT),
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
 *                       |      |      | UPPER|      |      |    |      |      |      |      |      |
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
 * ADJUST -- held via a dedicated left-thumb key (next to Globe). RGB
 * toggle/mode/saturation/brightness/flags as taps; Hue and Speed on the
 * encoders (left = Hue, right = Speed).
 * Blank cells are transparent (same key as _BASE).
 * ,-----------------------------------------------------.                    ,-----------------------------------------------------.
 * |RGBTog|Mode- |Mode+ | Sat- | Sat+ | Bri- |                                | Bri+ |Flag- |Flag+ |      |      |      |
 * |------+------+------+------+------+------|                                |------+------+------+------+------+------|
 * |      |      |      |      |      |      |                                |      |      |      |      |      |      |
 * |------+------+------+------+------+------|                                |------+------+------+------+------+------|
 * |      |      |      |      |      |      |-------.                ,-------|      |      |      |      |      |      |
 * |------+------+------+------+------+------|       |                |       |------+------+------+------+------+------|
 * |      |      |      |      |      |      |-------|                |-------|      |      |      |      |      |      |
 * `-----------------------------------------/       /                \      \-----------------------------------------'
 *                       |      | Adj  |      |      |      |    |      |      |      |      |      |
 *                       `----------------------------------'              '------''---------------------------'
 */
    [_ADJUST] = LAYOUT(
        RM_TOGG,  RM_PREV, RM_NEXT,RM_SATD, RM_SATU, RM_VALD,                                     RM_VALU, RM_FLGP, RM_FLGN,  KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,                                     KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,                                     KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,          KC_TRNS,           KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS,
                                    KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    ),
};

#if defined(ENCODER_MAP_ENABLE)
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    // Right encoder sends F14/F15, bound to brightness down/up on macOS.
    [_BASE]   = { ENCODER_CCW_CW(KC_VOLD, KC_VOLU), ENCODER_CCW_CW(KC_F14, KC_F15) },
    [_LOWER]  = { ENCODER_CCW_CW(KC_TRNS, KC_TRNS), ENCODER_CCW_CW(KC_TRNS, KC_TRNS) },
    [_UPPER]  = { ENCODER_CCW_CW(KC_TRNS, KC_TRNS), ENCODER_CCW_CW(KC_TRNS, KC_TRNS) },
    [_ADJUST] = { ENCODER_CCW_CW(RM_HUED, RM_HUEU), ENCODER_CCW_CW(RM_SPDD, RM_SPDU) },
};
#endif
