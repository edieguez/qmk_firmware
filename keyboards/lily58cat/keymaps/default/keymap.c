// Copyright 2026 edieguez
// SPDX-License-Identifier: GPL-2.0-or-later
#include QMK_KEYBOARD_H

enum layers {
    _BASE,
    _LOWER,
    _UPPER,
    _ADJUST,
};

enum {
    TD_CAPS = 0, // Caps Word on tap, Caps Lock on double tap
};

enum custom_keycodes {
    GRV_SPC = SAFE_RANGE, // ` on its own, see process_record_user()
};

// Spanish characters through AltGr, which US International maps them to.
// Right Alt isn't on the keyboard any more (see HM_SCLN), so these keys
// send it themselves. Shift gives Ñ.
#define ES_NTIL RALT(KC_N)    // ñ
#define ES_IQUE RALT(KC_SLSH) // ¿
#define ES_IEXL RALT(KC_1)    // ¡

// Backspace on tap, Upper on hold. Tap, then hold again to auto-repeat.
// Kept out of get_permissive_hold(): a backspace rolled into the next
// letter has to stay a tap.
#define UPP_BSPC LT(_UPPER, KC_BSPC)

// Home row mods. Ctrl sits where Cmd was on the Mac, so Ctrl+C/V/Z use the
// same fingers.
#define HM_A LALT_T(KC_A)
#define HM_S LGUI_T(KC_S)
#define HM_D LCTL_T(KC_D)
#define HM_F LSFT_T(KC_F)
#define HM_J RSFT_T(KC_J)
#define HM_K RCTL_T(KC_K)
#define HM_L RGUI_T(KC_L)
// Left Alt on the right hand too: Linux layouts treat Right Alt as AltGr, so
// RAlt+Tab was a plain Tab.
#define HM_SCLN LALT_T(KC_SCLN)

// ACTION_TAP_DANCE_DOUBLE sends the second tap with register_code16(), which
// bypasses process_record, so CW_TOGG can't be used there. Toggle Caps Word
// directly instead. One tap: Caps Word, or Caps Lock off if it was on. Two
// taps: Caps Lock.
static bool caps_dance_sent_capslock = false; // so the release matches the press

static void caps_dance_finished(tap_dance_state_t *state, void *user_data) {
    if (state->count == 1 && !host_keyboard_led_state().caps_lock) {
        caps_word_toggle();
        caps_dance_sent_capslock = false;
    } else {
        caps_word_off(); // its Shift would turn Caps Lock letters lowercase on Linux
        register_code(KC_CAPS);
        caps_dance_sent_capslock = true;
    }
}

static void caps_dance_reset(tap_dance_state_t *state, void *user_data) {
    if (caps_dance_sent_capslock) {
        unregister_code(KC_CAPS);
    }
}

tap_dance_action_t tap_dance_actions[] = {
    [TD_CAPS] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, caps_dance_finished, caps_dance_reset),
};

// The dance settles about TAPPING_TERM after keydown. If TD(TD_CAPS) counted
// as a word break, tapping it to turn Caps Word off would cancel it at once
// and the dance would then turn it back on.
bool caps_word_press_user(uint16_t keycode) {
    switch (keycode) {
        case TD(TD_CAPS):
            return true;
        case KC_A ... KC_Z:
        case ES_NTIL:
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

// US International makes ` and ~ dead keys that wait for the next letter,
// so these send the symbol followed by a space to get it on its own.
//   Shift+Esc -> ~    Alt+' -> `    Lower+Esc -> `
// Done here rather than with Key Overrides, which can't send two keys.
static void tap_dead_key(uint16_t keycode) {
    uint8_t mods = get_mods();
    del_mods(MOD_MASK_ALT);
    tap_code16(keycode);
    tap_code(KC_SPC);
    set_mods(mods);
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    // The press decides, and the release has to be swallowed with it.
    static bool esc_was_tilde  = false;
    static bool quot_was_grave = false;
    uint8_t     mods           = get_mods() | get_speculative_mods();

    switch (keycode) {
        case KC_ESC:
            if (record->event.pressed) {
                esc_was_tilde = mods & MOD_MASK_SHIFT;
                if (esc_was_tilde) {
                    tap_dead_key(S(KC_GRV));
                }
            }
            return !esc_was_tilde;
        case KC_QUOT:
            if (record->event.pressed) {
                quot_was_grave = mods & MOD_MASK_ALT;
                if (quot_was_grave) {
                    tap_dead_key(KC_GRV);
                }
            }
            return !quot_was_grave;
        case GRV_SPC:
            if (record->event.pressed) {
                tap_dead_key(KC_GRV);
            }
            return false;
        default:
            return true;
    }
}

// Shift and Ctrl (F J D K) get their own tap-hold tuning below.
static bool is_shift_ctrl_hrm(uint16_t keycode) {
    return keycode == HM_F || keycode == HM_J || keycode == HM_D || keycode == HM_K;
}

// Flow Tap would settle Shift/Ctrl as taps before Chordal Hold sees the next
// key, which broke fast Shift+/ and Ctrl+V. The Alt/Super keys keep it.
uint16_t get_flow_tap_term(uint16_t keycode, keyrecord_t *record, uint16_t prev_keycode) {
    return !is_shift_ctrl_hrm(keycode) && is_flow_tap_key(keycode) && is_flow_tap_key(prev_keycode) ? FLOW_TAP_TERM : 0;
}

// Shift/Ctrl chords often have almost no key overlap at speed, so these four
// get extra time for the chord to register. +80 wasn't enough. Hold On Other
// Key Press fixed the misses but fired on plain bigrams ("fi" -> "I").
uint16_t get_tapping_term(uint16_t keycode, keyrecord_t *record) {
    return is_shift_ctrl_hrm(keycode) ? TAPPING_TERM + 150 : TAPPING_TERM;
}

// Shift/Ctrl only. With it on Alt/Super, bigrams rolled across hands settled
// as held ("also" -> "aso").
bool get_permissive_hold(uint16_t keycode, keyrecord_t *record) {
    return is_shift_ctrl_hrm(keycode);
}

// Shift/Ctrl, so mouse chords (Shift-click, Ctrl-click) are instant.
bool get_speculative_hold(uint16_t keycode, keyrecord_t *record) {
    return is_shift_ctrl_hrm(keycode);
}

// Thumb and inner keys are exempt from the opposite-hands rule, or a home row
// mod plus a same-side thumb (Ctrl+Space, Ctrl+Enter) settles as a tap.
const char chordal_hold_layout[MATRIX_ROWS][MATRIX_COLS] PROGMEM = LAYOUT(
    'L', 'L', 'L', 'L', 'L', 'L',           'R', 'R', 'R', 'R', 'R', 'R',
    'L', 'L', 'L', 'L', 'L', 'L',           'R', 'R', 'R', 'R', 'R', 'R',
    'L', 'L', 'L', 'L', 'L', 'L',           'R', 'R', 'R', 'R', 'R', 'R',
    'L', 'L', 'L', 'L', 'L', 'L', '*', '*', 'R', 'R', 'R', 'R', 'R', 'R',
                   '*', '*', '*', '*', '*', '*', '*', '*'
);

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
/*
 * BASE. Home row mods on A S D F and J K L ;. Lower and Adjust are left
 * thumb holds, Upper is held Backspace. Backtick is Alt+' or Lower+Esc,
 * tilde is Shift+Esc. Caps: tap for Caps Word, double-tap for Caps Lock.
 * ,-----------------------------------------.                    ,-----------------------------------------.
 * | Esc  |   1  |   2  |   3  |   4  |   5  |                    |   6  |   7  |   8  |   9  |   0  |   -  |
 * |------+------+------+------+------+------|                    |------+------+------+------+------+------|
 * | Tab  |   Q  |   W  |   E  |   R  |   T  |                    |   Y  |   U  |   I  |   O  |   P  |   \  |
 * |------+------+------+------+------+------|                    |------+------+------+------+------+------|
 * |CW/Cap|A/LAlt|S/LSup|D/LCtl|F/LSft|   G  |-------.    ,-------|   H  |J/RSft|K/RCtl|L/RSup|;/LAlt|   '  |
 * |------+------+------+------+------+------|  Del  |    | Play  |------+------+------+------+------+------|
 * |   [  |   Z  |   X  |   C  |   V  |   B  |-------|    |-------|   N  |   M  |   ,  |   .  |   /  |   ]  |
 * `-----------------------------------------/       /     \      \-----------------------------------------'
 *                   | PrtSc|ADJUST|LOWER | /Space  /       \Enter \  |Bs/Upp|   =  | Super|
 *                   `----------------------------'         '------''--------------------'
 */
    [_BASE] = LAYOUT(
        KC_ESC,      KC_1,    KC_2,    KC_3,    KC_4,    KC_5,                          KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_MINS,
        KC_TAB,      KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,                          KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_BSLS,
        TD(TD_CAPS), HM_A,    HM_S,    HM_D,    HM_F,    KC_G,                          KC_H,    HM_J,    HM_K,    HM_L,    HM_SCLN, KC_QUOT,
        KC_LBRC,     KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_DEL,      KC_MPLY, KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH, KC_RBRC,
                         KC_PSCR, MO(_ADJUST), MO(_LOWER), KC_SPC,     KC_ENT,  UPP_BSPC, KC_EQL, KC_LGUI
    ),

/*
 * LOWER. Arrows on H J K L, word left and right above them, line start/end
 * and pages on the outer column. Backspace removes a word, Enter sends
 * Shift+Enter. ñ on N, ¿ on /, ¡ on 1.
 * ,-----------------------------------------.                    ,-----------------------------------------.
 * |   `  |   ¡  |      |      |      |      |                    |      |      |      |      |      | Home |
 * |------+------+------+------+------+------|                    |------+------+------+------+------+------|
 * |      |      |      |      |      |      |                    | wLeft|      |      |wRight|      | End  |
 * |------+------+------+------+------+------|                    |------+------+------+------+------+------|
 * |      |      |      |      |      |      |-------.    ,-------| Left | Down |  Up  | Right|      | PgUp |
 * |------+------+------+------+------+------|       |    |       |------+------+------+------+------+------|
 * |      |      |      |      |      |      |-------|    |-------|   ñ  |      |      |      |   ¿  | PgDn |
 * `-----------------------------------------/       /     \      \-----------------------------------------'
 *                   |      |      |LOWER | /       /       \S+Ent \  | wBspc|      |      |
 *                   `----------------------------'         '------''--------------------'
 */
    [_LOWER] = LAYOUT(
        GRV_SPC, ES_IEXL, _______, _______, _______, _______,                       _______,    _______, _______, _______,    _______, KC_HOME,
        _______, _______, _______, _______, _______, _______,                       C(KC_LEFT), _______, _______, C(KC_RGHT), _______, KC_END,
        _______, _______, _______, _______, _______, _______,                       KC_LEFT,    KC_DOWN, KC_UP,   KC_RGHT,    _______, KC_PGUP,
        _______, _______, _______, _______, _______, _______, _______,     _______, ES_NTIL,    _______, _______, _______,    ES_IQUE, KC_PGDN,
                                   _______, _______, _______, _______,     S(KC_ENT), C(KC_BSPC), _______, _______
    ),

/*
 * UPPER. F1-F5 on the number row, F6-F10 on Q-T, F11/F12 on D/F, all under
 * the left hand since the layer is a right-thumb hold.
 * ,-----------------------------------------.                    ,-----------------------------------------.
 * |      |  F1  |  F2  |  F3  |  F4  |  F5  |                    |      |      |      |      |      |      |
 * |------+------+------+------+------+------|                    |------+------+------+------+------+------|
 * |      |  F6  |  F7  |  F8  |  F9  |  F10 |                    |      |      |      |      |      |      |
 * |------+------+------+------+------+------|                    |------+------+------+------+------+------|
 * |      |      |      |  F11 |  F12 |      |-------.    ,-------|      |      |      |      |      |      |
 * |------+------+------+------+------+------|       |    |       |------+------+------+------+------+------|
 * |      |      |      |      |      |      |-------|    |-------|      |      |      |      |      |      |
 * `-----------------------------------------/       /     \      \-----------------------------------------'
 *                   |      |      |      | /       /       \      \  |UPPER |      |      |
 *                   `----------------------------'         '------''--------------------'
 */
    [_UPPER] = LAYOUT(
        _______, KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,                         _______, _______, _______, _______, _______, _______,
        _______, KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,                        _______, _______, _______, _______, _______, _______,
        _______, _______, _______, KC_F11,  KC_F12,  _______,                       _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______,     _______, _______, _______, _______, _______, _______, _______,
                                   _______, _______, _______, _______,     _______, _______, _______, _______
    ),

/*
 * ADJUST. Held on the left thumb, so everything but the underglow toggle is
 * under the right hand. Underglow settings are vertical pairs, + above -:
 * effect, speed, hue, saturation, LED brightness. The outer column holds
 * the levels: volume and screen brightness. Media on J K L, Mute on the
 * inner key.
 * ,-----------------------------------------.                    ,-----------------------------------------.
 * |RGBTog|      |      |      |      |      |                    | Mode+| Spd+ | Hue+ | Sat+ | LED+ | Vol+ |
 * |------+------+------+------+------+------|                    |------+------+------+------+------+------|
 * |      |      |      |      |      |      |                    | Mode-| Spd- | Hue- | Sat- | LED- | Vol- |
 * |------+------+------+------+------+------|                    |------+------+------+------+------+------|
 * |      |      |      |      |      |      |-------.    ,-------|      | Prev | Play | Next |      | Bri+ |
 * |------+------+------+------+------+------|       |    | Mute  |------+------+------+------+------+------|
 * |      |      |      |      |      |      |-------|    |-------|      |      |      |      |      | Bri- |
 * `-----------------------------------------/       /     \      \-----------------------------------------'
 *                   |      |ADJUST|      | /       /       \      \  |      |      |      |
 *                   `----------------------------'         '------''--------------------'
 */
    [_ADJUST] = LAYOUT(
        UG_TOGG, _______, _______, _______, _______, _______,                       UG_NEXT, UG_SPDU, UG_HUEU, UG_SATU, UG_VALU, KC_VOLU,
        _______, _______, _______, _______, _______, _______,                       UG_PREV, UG_SPDD, UG_HUED, UG_SATD, UG_VALD, KC_VOLD,
        _______, _______, _______, _______, _______, _______,                       _______, KC_MPRV, KC_MPLY, KC_MNXT, _______, KC_BRIU,
        _______, _______, _______, _______, _______, _______, _______,     KC_MUTE, _______, _______, _______, _______, _______, KC_BRID,
                                   _______, _______, _______, _______,     _______, _______, _______, _______
    ),
};
