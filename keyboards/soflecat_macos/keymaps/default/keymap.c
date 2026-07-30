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
        default:
            return true;
    }
}

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
    [TD_CAPS] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, caps_dance_finished, caps_dance_reset),
};

// Option+Backspace sends Delete on every layer, so a dedicated Del key isn't
// needed anywhere in the keymap. This doesn't fire on the _NAV layer's own
// Backspace key, which sends Option+Backspace (word delete) baked in as its
// own keycode rather than as a live Option+physical-Backspace chord, so the
// two don't collide.
const key_override_t delete_key_override = ko_make_basic(MOD_MASK_ALT, KC_BSPC, KC_DEL);

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
const key_override_t lbrc_key_override  = ko_make_basic(MOD_MASK_ALT, KC_I, KC_LBRC);
const key_override_t rbrc_key_override  = ko_make_basic(MOD_MASK_ALT, KC_O, KC_RBRC);

const key_override_t *key_overrides[] = {
    &delete_key_override,
    &tilde_esc_override,
    &equal_key_override,
    &lbrc_key_override,
    &rbrc_key_override,
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
/*
 * BASE -- the Globe thumb key is MACOS_GLOBE, a custom keycode (see above),
 * not a plain keycode.
 * ,-----------------------------------------------------.                    ,-----------------------------------------------------.
 * | Esc  |   1  |   2  |   3  |   4  |   5  |                                 |   6  |   7  |   8  |   9  |   0  |   -  |
 * |------+------+------+------+------+------|                                |------+------+------+------+------+------|
 * | Tab  |   Q  |   W  |   E  |   R  |   T  |                                 |   Y  |   U  |   I  |   O  |   P  |  \   |
 * |------+------+------+------+------+------|                                |------+------+------+------+------+------|
 * |Caps/CW|A/LAlt|S/LCtl|D/LGui|F/LSft|   G  |-------.                 ,-------|   H  |J/RSft|K/RGui|L/RCtl|;/RAlt|  '   |
 * |------+------+------+------+------+------|  Mute |                | Pause |------+------+------+------+------+------|
 * |OS_Sft|   Z  |   X  |   C  |   V  |   B  |-------|                |-------|   N  |   M  |   ,  |   .  |   /  |OS_Sft|
 * `-----------------------------------------/       /                \      \-----------------------------------------'
 *                       | Globe|  No  |  No  | NAV  | Space|    |Enter | Bspc |  FN  | Adj  |  No  |
 *                       `----------------------------------'              '------''---------------------------'
 */
    [_BASE] = LAYOUT(
        KC_ESC,      KC_1,          KC_2,          KC_3,          KC_4,          KC_5,                                          KC_6,          KC_7,          KC_8,          KC_9,          KC_0,            KC_MINUS,
        KC_TAB,      KC_Q,          KC_W,          KC_E,          KC_R,          KC_T,                                          KC_Y,          KC_U,          KC_I,          KC_O,          KC_P,            KC_BSLS,
        TD(TD_CAPS), LALT_T(KC_A),  LCTL_T(KC_S),  LGUI_T(KC_D),  LSFT_T(KC_F),  KC_G,                                          KC_H,          RSFT_T(KC_J),  RGUI_T(KC_K),  RCTL_T(KC_L),  RALT_T(KC_SCLN), KC_QUOTE,
        OSM(MOD_LSFT), KC_Z,      KC_X,          KC_C,          KC_V,          KC_B,    KC_MUTE,             KC_MPLY,        KC_N,          KC_M,          KC_COMMA,      KC_DOT,        KC_SLASH,        OSM(MOD_RSFT),
                                                    MACOS_GLOBE,   KC_NO,         KC_NO,    MO(_NAV), KC_SPACE, KC_ENTER, KC_BSPC, MO(_FN), MO(_ADJUST),   KC_NO
    ),

/*
 * NAV -- arrows plus macOS-native word/line/document navigation stacked
 * around the arrow cluster: Option+Left/Right (word jump) above, and
 * Option+Delete (word delete) next to it. macOS's own Home/End/PgUp/PgDn
 * equivalents -- Cmd+Left/Right (line start/end) and Cmd+Up/Down (document
 * start/end) -- live in the outer column, in that order, standing in for the
 * raw Home/End/PgUp/PgDn keycodes (which macOS handles inconsistently). The
 * thumb-cluster Backspace key sends Option+Backspace here (word delete
 * backwards), and the thumb-cluster Enter key sends Shift+Enter (newline
 * without submitting, for chat apps), each overriding its _BASE meaning.
 * Blank cells are transparent (same key as _BASE).
 * ,-----------------------------------------------------.                    ,-----------------------------------------------------.
 * |      |      |      |      |      |      |                                 |      |      |      |      |      |LnStrt|
 * |------+------+------+------+------+------|                                |------+------+------+------+------+------|
 * |      |      |      |      |      |      |                                 | wLeft|      |      |wRight|     | LnEnd|
 * |------+------+------+------+------+------|                                |------+------+------+------+------+------|
 * |      |      |      |      |      |      |-------.                 ,-------|  Left|  Down|   Up | Right|      |DocTop|
 * |------+------+------+------+------+------|       |                |       |------+------+------+------+------+------|
 * |      |      |      |      |      |      |-------|                |-------|      |      |      |      |      |DocEnd|
 * `-----------------------------------------/       /                \      \-----------------------------------------'
 *                       |      |      |      |      |      |    |ShfEnt| wBspc|      |      |      |
 *                       `----------------------------------'              '------''---------------------------'
 */
    [_NAV] = LAYOUT(
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,                                     KC_TRNS,    KC_TRNS, KC_TRNS,   KC_TRNS,     KC_TRNS,    G(KC_LEFT),
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,                                     A(KC_LEFT), KC_TRNS, KC_TRNS,   A(KC_RIGHT), KC_TRNS,   G(KC_RIGHT),
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,                                     KC_LEFT,    KC_DOWN, KC_UP,     KC_RIGHT,    KC_TRNS,    G(KC_UP),
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,          KC_TRNS,           KC_TRNS,    KC_TRNS,    KC_TRNS, KC_TRNS,     KC_TRNS,   G(KC_DOWN),
                                    KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, S(KC_ENTER),  A(KC_BSPC), KC_TRNS, KC_TRNS, KC_TRNS
    ),

/*
 * FN -- F1-F12 on the number row (F11/F12 on T/Y). Blank cells are
 * transparent (same key as _BASE).
 * ,-----------------------------------------------------.                    ,-----------------------------------------------------.
 * |      |  F1  |  F2  |  F3  |  F4  |  F5  |                                 |  F6  |  F7  |  F8  |  F9  |  F10 |      |
 * |------+------+------+------+------+------|                                |------+------+------+------+------+------|
 * |      |      |      |      |      |  F11 |                                 |  F12 |      |      |      |      |      |
 * |------+------+------+------+------+------|                                |------+------+------+------+------+------|
 * |      |      |      |      |      |      |-------.                 ,-------|      |      |      |      |      |      |
 * |------+------+------+------+------+------|       |                |       |------+------+------+------+------+------|
 * |      |      |      |      |      |      |-------|                |-------|      |      |      |      |      |      |
 * `-----------------------------------------/       /                \      \-----------------------------------------'
 *                       |      |      |      |      |      |    |      |      |      |      |      |
 *                       `----------------------------------'              '------''---------------------------'
 */
    [_FN] = LAYOUT(
        KC_TRNS,  KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,                                       KC_F6,   KC_F7,   KC_F8,    KC_F9,   KC_F10,  KC_TRNS,
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_F11,                                       KC_F12,  KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,                                     KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,          KC_TRNS,           KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS,
                                    KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    ),

/*
 * ADJUST -- held via the right-outer thumb key. RGB toggle/mode/saturation/
 * brightness/flags as taps; Hue and Speed on the encoders (left = Hue,
 * right = Speed). QK_BOOT reflashes without the physical reset button.
 * Blank cells are transparent (same key as _BASE).
 * ,-----------------------------------------------------.                    ,-----------------------------------------------------.
 * |RGBTog|Mode- |Mode+ | Sat- | Sat+ | Bri- |                                 | Bri+ |Flag+ |Flag- |      |      | Boot |
 * |------+------+------+------+------+------|                                |------+------+------+------+------+------|
 * |      |      |      |      |      |      |                                 |      |      |      |      |      |      |
 * |------+------+------+------+------+------|                                |------+------+------+------+------+------|
 * |      |      |      |      |      |      |-------.                 ,-------|      |      |      |      |      |      |
 * |------+------+------+------+------+------|       |                |       |------+------+------+------+------+------|
 * |      |      |      |      |      |      |-------|                |-------|      |      |      |      |      |      |
 * `-----------------------------------------/       /                \      \-----------------------------------------'
 *                       |      |      |      |      |      |    |      |      |      |      |      |
 *                       `----------------------------------'              '------''---------------------------'
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
    // Right encoder sends F14/F15, bound to brightness down/up on macOS.
    [_BASE]   = { ENCODER_CCW_CW(KC_VOLD, KC_VOLU), ENCODER_CCW_CW(KC_F14, KC_F15) },
    [_NAV]    = { ENCODER_CCW_CW(KC_TRNS, KC_TRNS), ENCODER_CCW_CW(KC_TRNS, KC_TRNS) },
    [_FN]     = { ENCODER_CCW_CW(KC_TRNS, KC_TRNS), ENCODER_CCW_CW(KC_TRNS, KC_TRNS) },
    [_ADJUST] = { ENCODER_CCW_CW(RM_HUED, RM_HUEU), ENCODER_CCW_CW(RM_SPDD, RM_SPDU) },
};
#endif
