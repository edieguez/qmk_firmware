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
    _NAV,
    _FN,
    _ADJUST,
};

// Number row: tap for the digit, double-tap for the matching F-key.
enum {
    TD_NUM_1 = 0,
    TD_NUM_2,
    TD_NUM_3,
    TD_NUM_4,
    TD_NUM_5,
    TD_NUM_6,
    TD_NUM_7,
    TD_NUM_8,
    TD_NUM_9,
    TD_NUM_0,
    // Caps Lock on tap, Caps Word toggle on double-tap.
    TD_CAPS,
};

// ACTION_TAP_DANCE_DOUBLE's second tap is sent via register_code16(), which
// only ever produces a raw HID report -- it bypasses process_record(), so it
// can't reach quantum keycodes like CW_TOGG whose behavior lives entirely in
// a process_record hook (process_caps_word.c). Caps Word's double-tap must
// instead call caps_word_toggle() directly, hence the "advanced" dance below
// instead of ACTION_TAP_DANCE_DOUBLE(KC_CAPS, CW_TOGG).
static void caps_dance_finished(tap_dance_state_t *state, void *user_data) {
    if (state->count == 1) {
        register_code(KC_CAPS);
    } else {
        caps_word_toggle();
    }
}

static void caps_dance_reset(tap_dance_state_t *state, void *user_data) {
    if (state->count == 1) {
        unregister_code(KC_CAPS);
    }
}

tap_dance_action_t tap_dance_actions[] = {
    [TD_NUM_1] = ACTION_TAP_DANCE_DOUBLE(KC_1, KC_F1),
    [TD_NUM_2] = ACTION_TAP_DANCE_DOUBLE(KC_2, KC_F2),
    [TD_NUM_3] = ACTION_TAP_DANCE_DOUBLE(KC_3, KC_F3),
    [TD_NUM_4] = ACTION_TAP_DANCE_DOUBLE(KC_4, KC_F4),
    [TD_NUM_5] = ACTION_TAP_DANCE_DOUBLE(KC_5, KC_F5),
    [TD_NUM_6] = ACTION_TAP_DANCE_DOUBLE(KC_6, KC_F6),
    [TD_NUM_7] = ACTION_TAP_DANCE_DOUBLE(KC_7, KC_F7),
    [TD_NUM_8] = ACTION_TAP_DANCE_DOUBLE(KC_8, KC_F8),
    [TD_NUM_9] = ACTION_TAP_DANCE_DOUBLE(KC_9, KC_F9),
    [TD_NUM_0] = ACTION_TAP_DANCE_DOUBLE(KC_0, KC_F10),
    [TD_CAPS]  = ACTION_TAP_DANCE_FN_ADVANCED(NULL, caps_dance_finished, caps_dance_reset),
};

// Shift+Backspace sends Delete on every layer, so a dedicated Del key isn't
// needed anywhere in the keymap (see the _NAV layer).
const key_override_t delete_key_override = ko_make_basic(MOD_MASK_SHIFT, KC_BSPC, KC_DEL);
const key_override_t *key_overrides[]     = {
    &delete_key_override,
};

// Shorter tapping term for the home row mods only, so they resolve to a tap
// quickly during normal typing; everything else keeps the default TAPPING_TERM.
uint16_t get_tapping_term(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case LALT_T(KC_A):
        case LCTL_T(KC_S):
        case LGUI_T(KC_D):
        case LSFT_T(KC_F):
        case RSFT_T(KC_J):
        case RGUI_T(KC_K):
        case RCTL_T(KC_L):
        case RALT_T(KC_SCLN):
            return 149;
        default:
            return TAPPING_TERM;
    }
}

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
/*
 * BASE
 * ,-----------------------------------------------------.                    ,-----------------------------------------------------.
 * |  ~`  |  1/F1|  2/F2|  3/F3|  4/F4|  5/F5|                                 |  6/F6|  7/F7|  8/F8|  9/F9| 0/F10|   -  |
 * |------+------+------+------+------+------|                                |------+------+------+------+------+------|
 * | Tab  |   Q  |   W  |   E  |   R  |   T  |                                 |   Y  |   U  |   I  |   O  |   P  |  \   |
 * |------+------+------+------+------+------|                                |------+------+------+------+------+------|
 * |Caps/CW|A/LAlt|S/LCtl|D/LGui|F/LSft|   G  |-------.                 ,-------|   H  |J/RSft|K/RGui|L/RCtl|;/RAlt|  '   |
 * |------+------+------+------+------+------|  Mute |                | Pause |------+------+------+------+------+------|
 * |OS_Sft|   Z  |   X  |   C  |   V  |   B  |-------|                |-------|   N  |   M  |   ,  |   .  |   /  |OS_Sft|
 * `-----------------------------------------/       /                \      \-----------------------------------------'
 *                       |      | NAV  | LGUI | Space| GEsc |    |Enter | Bspc | RGUI | FN   | Adj  |
 *                       `----------------------------------'              '------''---------------------------'
 */
    [_BASE] = LAYOUT(
        KC_GRV,      TD(TD_NUM_1),  TD(TD_NUM_2),  TD(TD_NUM_3),  TD(TD_NUM_4),  TD(TD_NUM_5),                                  TD(TD_NUM_6),  TD(TD_NUM_7),  TD(TD_NUM_8),  TD(TD_NUM_9),  TD(TD_NUM_0),    KC_MINUS,
        KC_TAB,      KC_Q,          KC_W,          KC_E,          KC_R,          KC_T,                                          KC_Y,          KC_U,          KC_I,          KC_O,          KC_P,            KC_BSLS,
        TD(TD_CAPS), LALT_T(KC_A),  LCTL_T(KC_S),  LGUI_T(KC_D),  LSFT_T(KC_F),  KC_G,                                          KC_H,          RSFT_T(KC_J),  RGUI_T(KC_K),  RCTL_T(KC_L),  RALT_T(KC_SCLN), KC_QUOTE,
        OSM(MOD_LSFT), KC_Z,      KC_X,          KC_C,          KC_V,          KC_B,    KC_MUTE,             KC_MPLY,        KC_N,          KC_M,          KC_COMMA,      KC_DOT,        KC_SLASH,        OSM(MOD_RSFT),
                                                    KC_NO,         MO(_NAV),      KC_LGUI,  KC_SPACE, QK_GESC, KC_ENTER, KC_BSPC, KC_RGUI, MO(_FN),       MO(_ADJUST)
    ),

/*
 * NAV -- arrows/Home/End/PgUp/PgDn (Delete is available everywhere via the
 * Shift+Backspace key override, so it doesn't need its own slot here)
 */
    [_NAV] = LAYOUT(
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,                                     KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_HOME,
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,                                     KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_END,
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,                                     KC_LEFT, KC_DOWN, KC_UP,    KC_RIGHT,KC_TRNS, KC_PGUP,
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,          KC_TRNS,           KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_PGDN,
                                    KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    ),

/*
 * FN -- F11/F12, [ and ] under I/O (Shift+ gives { and }), = under - (Shift+ gives +)
 */
    [_FN] = LAYOUT(
        KC_TRNS,  KC_F11,  KC_F12,  KC_TRNS, KC_TRNS, KC_TRNS,                                     KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_EQUAL,
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,                                     KC_TRNS, KC_TRNS, KC_LBRC,  KC_RBRC, KC_TRNS, KC_TRNS,
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,                                     KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,          KC_TRNS,           KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS,
                                    KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    ),

/*
 * ADJUST -- held via the right-outer thumb key. RGB toggle/mode/saturation/
 * brightness/flags as taps; Hue and Speed on the encoders (left = Hue,
 * right = Speed). QK_BOOT reflashes without the physical reset button.
 */
    [_ADJUST] = LAYOUT(
        RM_TOGG,  RM_PREV, RM_NEXT,RM_SATD, RM_SATU, RM_VALD,                                     RM_VALU, RM_FLGP, RM_FLGN,  KC_TRNS, KC_TRNS, QK_BOOT,
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,                                     KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,                                     KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,          KC_TRNS,           KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS,
                                    KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    ),
};

#if defined(ENCODER_MAP_ENABLE)
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    // Right encoder sends F14/F15, bound to volume down/up on macOS.
    [_BASE]   = { ENCODER_CCW_CW(KC_VOLD, KC_VOLU), ENCODER_CCW_CW(KC_F14, KC_F15) },
    [_NAV]    = { ENCODER_CCW_CW(KC_TRNS, KC_TRNS), ENCODER_CCW_CW(KC_TRNS, KC_TRNS) },
    [_FN]     = { ENCODER_CCW_CW(KC_TRNS, KC_TRNS), ENCODER_CCW_CW(KC_TRNS, KC_TRNS) },
    [_ADJUST] = { ENCODER_CCW_CW(RM_HUED, RM_HUEU), ENCODER_CCW_CW(RM_SPDD, RM_SPDU) },
};
#endif
