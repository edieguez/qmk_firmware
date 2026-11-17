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
#include <string.h>

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
    // macOS Globe key. QMK has no keycode for consumer usage 0x29D (KC_LNG1
    // is something else), so it's sent by hand below.
    MACOS_GLOBE = SAFE_RANGE,
    ES_NTIL, // ñ and ü: Option dead key, then the letter
    ES_UDIA,
    // Home row mod tapping terms, see hrm_tuning. Selectors in the order of
    // enum hrm_mod, the knob steps, then show and reset.
    HRM_SEL_ALT,
    HRM_SEL_CTRL,
    HRM_SEL_CMD,
    HRM_SEL_SHIFT,
    HRM_KNOB_UP,
    HRM_KNOB_DN,
    HRM_SHOW,
    HRM_RESET,
};

#define ES_IQUE A(S(KC_SLSH)) // ¿
#define ES_IEXL A(KC_1)       // ¡

// Backspace on tap, Upper on hold. Tap, then hold again to auto-repeat.
// Kept out of get_permissive_hold(): a backspace rolled into the next
// letter has to stay a tap.
#define UPP_BSPC LT(_UPPER, KC_BSPC)

// Each home row modifier's tapping term is adjustable from Adjust: tap A, S,
// D or F to pick Alt/Ctrl/Cmd/Shift, then turn the left knob (5 ms per
// detent). G shows all four, B resets them. The terms are kept in the user
// EEPROM block, so they survive reboots; the selection isn't, and starts at
// Shift. QMK's Dynamic Tapping Term wasn't used: it moves the one global
// term and doesn't save it.
enum hrm_mod { HRM_ALT, HRM_CTRL, HRM_CMD, HRM_SHIFT, HRM_MOD_COUNT };

#define HRM_TERM_STEP 5
#define HRM_TERM_MIN 100
#define HRM_TERM_MAX 500
// Shift/Cmd chords often have almost no key overlap at speed, so they get
// extra time for the chord to register. +80 wasn't enough. Hold On Other Key
// Press fixed the misses but fired on plain bigrams ("fi" -> "I").
#define HRM_TERM_DEFAULTS \
    { [HRM_ALT] = TAPPING_TERM, [HRM_CTRL] = TAPPING_TERM, [HRM_CMD] = TAPPING_TERM + 150, [HRM_SHIFT] = TAPPING_TERM + 150 }

static const char *const hrm_mod_name[HRM_MOD_COUNT] = {"ALT", "CTRL", "CMD", "SHIFT"};

typedef struct {
    uint16_t term[HRM_MOD_COUNT];
} hrm_tuning_t;
_Static_assert(sizeof(hrm_tuning_t) == EECONFIG_USER_DATA_SIZE, "hrm_tuning_t must fill the user EEPROM block");

static hrm_tuning_t hrm_tuning  = {.term = HRM_TERM_DEFAULTS};
static enum hrm_mod hrm_selected = HRM_SHIFT; // what the knob adjusts

static void hrm_tuning_save(void) {
    eeconfig_update_user_datablock(&hrm_tuning, 0, sizeof(hrm_tuning));
}

// QMK calls this when the block is missing or EECONFIG_USER_DATA_VERSION
// changed.
void eeconfig_init_user_datablock(void) {
    static const uint16_t defaults[HRM_MOD_COUNT] = HRM_TERM_DEFAULTS;
    memcpy(hrm_tuning.term, defaults, sizeof(hrm_tuning.term));
    hrm_tuning_save();
}

void keyboard_post_init_user(void) {
    eeconfig_read_user_datablock(&hrm_tuning, 0, sizeof(hrm_tuning));
    for (uint8_t i = 0; i < HRM_MOD_COUNT; i++) {
        if (hrm_tuning.term[i] < HRM_TERM_MIN || hrm_tuning.term[i] > HRM_TERM_MAX) {
            eeconfig_init_user_datablock();
            return;
        }
    }
}

// Clamps, saves if it changed, and shows the value either way.
static void hrm_term_set(enum hrm_mod mod, uint16_t term) {
    if (term < HRM_TERM_MIN) {
        term = HRM_TERM_MIN;
    } else if (term > HRM_TERM_MAX) {
        term = HRM_TERM_MAX;
    }
    if (term != hrm_tuning.term[mod]) {
        hrm_tuning.term[mod] = term;
        hrm_tuning_save();
    }
    oled_show_value(hrm_mod_name[mod], hrm_tuning.term[mod]);
}

// All four, one per two OLED rows.
static void hrm_terms_show(void) {
    char text[48];
    snprintf(text, sizeof(text), "%s\n%u\n%s\n%u\n%s\n%u\n%s\n%u", hrm_mod_name[HRM_ALT], hrm_tuning.term[HRM_ALT], hrm_mod_name[HRM_CTRL], hrm_tuning.term[HRM_CTRL], hrm_mod_name[HRM_CMD], hrm_tuning.term[HRM_CMD], hrm_mod_name[HRM_SHIFT], hrm_tuning.term[HRM_SHIFT]);
    oled_show_message(text);
}

// Option+N / Option+U is a dead key on macOS; the letter after it gets the
// accent. Shift (or Caps Word, which never sees this key) only applies to
// the letter, since Option+Shift+N is a different character.
static void tap_mac_accent(uint16_t dead_key, uint8_t letter) {
    uint8_t mods  = get_mods();
    bool    upper = (mods | get_weak_mods()) & MOD_MASK_SHIFT || is_caps_word_on();
    del_mods(MOD_MASK_SHIFT);
    del_weak_mods(MOD_MASK_SHIFT);
    tap_code16(dead_key);
    tap_code16(upper ? S(letter) : letter);
    set_mods(mods);
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case MACOS_GLOBE:
            if (record->event.pressed) {
                host_consumer_send(AC_NEXT_KEYBOARD_LAYOUT_SELECT);
            } else {
                host_consumer_send(0);
            }
            return false;
        case ES_NTIL:
        case ES_UDIA:
            if (record->event.pressed) {
                tap_mac_accent(A(keycode == ES_NTIL ? KC_N : KC_U), keycode == ES_NTIL ? KC_N : KC_U);
            }
            return false;
        case HRM_SEL_ALT ... HRM_SEL_SHIFT:
            if (record->event.pressed) {
                hrm_selected = keycode - HRM_SEL_ALT;
                oled_show_value(hrm_mod_name[hrm_selected], hrm_tuning.term[hrm_selected]);
            }
            return false;
        case HRM_KNOB_UP:
        case HRM_KNOB_DN:
            if (record->event.pressed) {
                hrm_term_set(hrm_selected, hrm_tuning.term[hrm_selected] + (keycode == HRM_KNOB_UP ? HRM_TERM_STEP : -HRM_TERM_STEP));
            }
            return false;
        case HRM_SHOW:
            if (record->event.pressed) {
                hrm_terms_show();
            }
            return false;
        case HRM_RESET:
            if (record->event.pressed) {
                eeconfig_init_user_datablock();
                hrm_terms_show();
            }
            return false;
        default:
            return true;
    }
}

// ACTION_TAP_DANCE_DOUBLE sends the second tap with register_code16(), which
// bypasses process_record, so CW_TOGG can't be used there. Toggle Caps Word
// directly instead. One tap: Caps Word, or Caps Lock off if it was on. Two
// taps: Caps Lock.
static bool caps_dance_sent_capslock = false; // so the release matches the press

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
        wait_ms(100); // macOS drops a Caps Lock press that releases too fast
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

// Flow Tap would settle Shift/Cmd as taps before Chordal Hold sees the next
// key, which broke fast Shift+/ and Cmd+V. The Alt/Ctrl keys keep it.
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

// Per-modifier terms from hrm_tuning. Everything else (the Caps dance, the
// layer taps) keeps TAPPING_TERM.
uint16_t get_tapping_term(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case LALT_T(KC_A):
        case RALT_T(KC_SCLN):
            return hrm_tuning.term[HRM_ALT];
        case LCTL_T(KC_S):
        case RCTL_T(KC_L):
            return hrm_tuning.term[HRM_CTRL];
        case LGUI_T(KC_D):
        case RGUI_T(KC_K):
            return hrm_tuning.term[HRM_CMD];
        case LSFT_T(KC_F):
        case RSFT_T(KC_J):
            return hrm_tuning.term[HRM_SHIFT];
        default:
            return TAPPING_TERM;
    }
}

// Shift/Cmd only. With it on Alt/Ctrl, bigrams rolled across hands settled
// as held ("also" -> "aso").
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

// Shift/Cmd only, so mouse chords (Shift-click, Cmd-click) are instant.
// Ctrl stays out: Ctrl+click is right-click on macOS.
bool get_speculative_hold(uint16_t keycode, keyrecord_t *record) {
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

// Shift+Esc = ~. Grave Escape turns Cmd+Esc into Cmd+`, which macOS eats, so
// it couldn't cancel Cmd+Tab.
const key_override_t tilde_esc_override = ko_make_basic(MOD_MASK_SHIFT, KC_ESC, S(KC_GRV));

// Option+' = `. No spare key for it.
const key_override_t grave_key_override = ko_make_basic(MOD_MASK_ALT, KC_QUOTE, KC_GRV);

const key_override_t *key_overrides[] = {
    &tilde_esc_override,
    &grave_key_override,
};

// Thumb keys and knob clicks are exempt from the opposite-hands rule, or a
// home row mod plus a same-side thumb (Cmd+Space, Cmd+Enter) settles as a tap.
const char chordal_hold_layout[MATRIX_ROWS][MATRIX_COLS] PROGMEM = LAYOUT(
    'L', 'L', 'L', 'L', 'L', 'L',                     'R', 'R', 'R', 'R', 'R', 'R',
    'L', 'L', 'L', 'L', 'L', 'L',                     'R', 'R', 'R', 'R', 'R', 'R',
    'L', 'L', 'L', 'L', 'L', 'L',                     'R', 'R', 'R', 'R', 'R', 'R',
    'L', 'L', 'L', 'L', 'L', 'L', '*',           '*', 'R', 'R', 'R', 'R', 'R', 'R',
                   '*', '*', '*', '*', '*',   '*', '*', '*', '*', '*'
);

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
/*
 * BASE. Home row mods on A S D F and J K L ;. Lower and Adjust are thumb
 * holds, Upper is held Backspace. Delete is Lower+Esc, backtick is Option+'.
 * ,-----------------------------------------------------.                    ,-----------------------------------------------------.
 * | Esc  |   1  |   2  |   3  |   4  |   5  |                                |   6  |   7  |   8  |   9  |   0  |   -  |
 * |------+------+------+------+------+------|                                |------+------+------+------+------+------|
 * | Tab  |   Q  |   W  |   E  |   R  |   T  |                                |   Y  |   U  |   I  |   O  |   P  |  \   |
 * |------+------+------+------+------+------|                                |------+------+------+------+------+------|
 * |CW/Caps|A/LAlt|S/LCtl|D/LGui|F/LSft|   G  |-------.                ,-------|   H  |J/RSft|K/RGui|L/RCtl|;/RAlt|  '   |
 * |------+------+------+------+------+------|  Mute |                | Play  |------+------+------+------+------+------|
 * |  [   |   Z  |   X  |   C  |   V  |   B  |-------|                |-------|   N  |   M  |   ,  |   .  |   /  |  ]   |
 * `-----------------------------------------/       /                \      \-----------------------------------------'
 *                       |      |      |Globe |LOWER | Space|    |Enter |Bs/Upp|  =   |      |ADJUST|
 *                       `----------------------------------'              '------''---------------------------'
 */
    [_BASE] = LAYOUT(
        KC_ESC,      KC_1,          KC_2,          KC_3,          KC_4,          KC_5,                                          KC_6,          KC_7,          KC_8,          KC_9,          KC_0,            KC_MINUS,
        KC_TAB,      KC_Q,          KC_W,          KC_E,          KC_R,          KC_T,                                          KC_Y,          KC_U,          KC_I,          KC_O,          KC_P,            KC_BSLS,
        TD(TD_CAPS), LALT_T(KC_A),  LCTL_T(KC_S),  LGUI_T(KC_D),  LSFT_T(KC_F),  KC_G,                                          KC_H,          RSFT_T(KC_J),  RGUI_T(KC_K),  RCTL_T(KC_L),  RALT_T(KC_SCLN), KC_QUOTE,
        KC_LBRC,     KC_Z,          KC_X,          KC_C,          KC_V,          KC_B,    KC_MUTE,             KC_MPLY,        KC_N,          KC_M,          KC_COMMA,      KC_DOT,        KC_SLASH,        KC_RBRC,
                                                    KC_NO,         KC_NO,        MACOS_GLOBE, MO(_LOWER), KC_SPACE, KC_ENTER, UPP_BSPC, KC_EQUAL, KC_NO,     MO(_ADJUST)
    ),

/*
 * LOWER. Arrows with word jumps beside them; Home, End, PgUp, PgDn on the
 * outer column. Backspace sends Option+Backspace, Enter sends Shift+Enter.
 * The right knob jumps by word.
 * ñ on N, ü on U, ¿ on /, ¡ on 1 (Option sequences); Delete on Esc.
 * ,-----------------------------------------------------.                    ,-----------------------------------------------------.
 * | Del  |   ¡  |      |      |      |      |                                |      |      |      |      |      | Home |
 * |------+------+------+------+------+------|                                |------+------+------+------+------+------|
 * |      |      |      |      |      |      |                                | wLeft|   ü  |      |wRight|      | End  |
 * |------+------+------+------+------+------|                                |------+------+------+------+------+------|
 * |      |      |      |      |      |      |-------.                ,-------|  Left|  Down|   Up | Right|      | PgUp |
 * |------+------+------+------+------+------|       |                |       |------+------+------+------+------+------|
 * |      |      |      |      |      |      |-------|                |-------|   ñ  |      |      |      |   ¿  | PgDn |
 * `-----------------------------------------/       /                \      \-----------------------------------------'
 *                       |      |      |      |LOWER |      |    |Sft+Enter| wBspc|      |      |      |
 *                       `----------------------------------'              '------''---------------------------'
 */
    [_LOWER] = LAYOUT(
        KC_DEL,   ES_IEXL, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,                                     KC_TRNS,    KC_TRNS, KC_TRNS,   KC_TRNS,     KC_TRNS,    KC_HOME,
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,                                     A(KC_LEFT), ES_UDIA, KC_TRNS,   A(KC_RIGHT), KC_TRNS,    KC_END,
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,                                     KC_LEFT,    KC_DOWN, KC_UP,     KC_RIGHT,    KC_TRNS,    KC_PGUP,
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,          KC_TRNS,           ES_NTIL,    KC_TRNS,    KC_TRNS, KC_TRNS,     ES_IQUE,    KC_PGDN,
                                    KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, S(KC_ENTER),    A(KC_BSPC), KC_TRNS, KC_TRNS, KC_TRNS
    ),

/*
 * UPPER. F1-F5 on the number row, F6-F10 on Q-T, F11/F12 on D/F, all under
 * the left hand since the layer is a right-thumb hold. Left knob zooms,
 * click resets.
 * ,-----------------------------------------------------.                    ,-----------------------------------------------------.
 * |      |  F1  |  F2  |  F3  |  F4  |  F5  |                                |      |      |      |      |      |      |
 * |------+------+------+------+------+------|                                |------+------+------+------+------+------|
 * |      |  F6  |  F7  |  F8  |  F9  |  F10 |                                |      |      |      |      |      |      |
 * |------+------+------+------+------+------|                                |------+------+------+------+------+------|
 * |      |      |      |  F11 |  F12 |      |-------.                ,-------|      |      |      |      |      |      |
 * |------+------+------+------+------+------| Zoom0 |                |       |------+------+------+------+------+------|
 * |      |      |      |      |      |      |-------|                |-------|      |      |      |      |      |      |
 * `-----------------------------------------/       /                \      \-----------------------------------------'
 *                       |      |      |      |      |      |    |      |UPPER |      |      |      |
 *                       `----------------------------------'    '------''---------------------------'
 */
    [_UPPER] = LAYOUT(
        KC_TRNS,  KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,                                       KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS,  KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,                                      KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_F11,  KC_F12,  KC_TRNS,                                     KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, G(KC_0),          KC_TRNS,           KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS,
                                    KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    ),

/*
 * ADJUST. RGB controls on the number row, Mission Control and Dictation on
 * W/R. Home row mod tapping terms: tap A, S, D or F to pick
 * Alt/Ctrl/Cmd/Shift, then turn the left knob (5 ms a detent); G shows all
 * four, B resets them; the values are kept in EEPROM. Left knob click: Do Not
 * Disturb. Right knob: previous/next track. Dictation (Ctrl+Opt+Cmd+D) and Do
 * Not Disturb (Ctrl+Opt+Cmd+N) are chords that have to be bound in System
 * Settings.
 * ,-----------------------------------------------------.                    ,-----------------------------------------------------.
 * |RGBTog|Mode- |Mode+ | Hue- | Hue+ |      |                                | Sat- | Sat+ | Bri- | Bri+ | Spd- | Spd+ |
 * |------+------+------+------+------+------|                                |------+------+------+------+------+------|
 * |      |      |MCtrl |      | Dict |      |                                |      |      |      |      |      |      |
 * |------+------+------+------+------+------|                                |------+------+------+------+------+------|
 * |      | Alt  | Ctrl | Cmd  | Sft  | Show |-------.                ,-------|      |      |      |      |      |      |
 * |------+------+------+------+------+------|  DND  |                |       |------+------+------+------+------+------|
 * |      |      |      |      |      |Reset |-------|                |-------|      |      |      |      |      |      |
 * `-----------------------------------------/       /                \      \-----------------------------------------'
 *                       |      |      |      |      |      |    |      |      |      |      |ADJUST|
 *                       `----------------------------------'              '------''---------------------------'
 */
    [_ADJUST] = LAYOUT(
        RM_TOGG,  RM_PREV, RM_NEXT, RM_HUED,   RM_HUEU,    KC_TRNS,                                     RM_SATD, RM_SATU, RM_VALD,  RM_VALU, RM_SPDD, RM_SPDU,
        KC_TRNS,  KC_TRNS, KC_MCTL, KC_TRNS,   LCAG(KC_D), KC_TRNS,                                     KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS,  HRM_SEL_ALT, HRM_SEL_CTRL, HRM_SEL_CMD, HRM_SEL_SHIFT, HRM_SHOW,                        KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS,   KC_TRNS,    HRM_RESET, LCAG(KC_N),       KC_TRNS,           KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS,
                                    KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    ),
};

#ifdef RGB_MATRIX_ENABLE
// Light the keys that do something on the held layer and blank the rest. On
// Adjust the rest keeps the running effect so you can see what you're
// adjusting. The slave half needs SPLIT_LAYER_STATE_ENABLE for this.
bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    uint8_t layer = get_highest_layer(layer_state);
    if (layer == _BASE) {
        return true;
    }
    uint8_t hue;
    switch (layer) {
        case _LOWER:  hue = 128; break; // cyan
        case _UPPER:  hue = 85;  break; // green
        default:      hue = 21;  break; // orange
    }
    RGB on = hsv_to_rgb((HSV){hue, 255, rgb_matrix_get_val()});

    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        for (uint8_t col = 0; col < MATRIX_COLS; col++) {
            uint8_t index = g_led_config.matrix_co[row][col];
            if (index < led_min || index >= led_max || index == NO_LED) {
                continue;
            }
            if (keymap_key_to_keycode(layer, (keypos_t){col, row}) > KC_TRNS) {
                rgb_matrix_set_color(index, on.r, on.g, on.b);
            } else if (layer != _ADJUST) {
                rgb_matrix_set_color(index, 0, 0, 0);
            }
        }
    }
    return false;
}
#endif

#if defined(ENCODER_MAP_ENABLE)
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    // Base: brightness (macOS maps F14/F15 to it) and volume. On the layers
    // the knob sits opposite the hand holding the layer; on Adjust the left
    // knob steps the selected home row mod's tapping term.
    [_BASE]   = { ENCODER_CCW_CW(KC_F14, KC_F15),     ENCODER_CCW_CW(KC_VOLD, KC_VOLU) },
    [_LOWER]  = { ENCODER_CCW_CW(KC_TRNS, KC_TRNS),   ENCODER_CCW_CW(A(KC_LEFT), A(KC_RIGHT)) },
    [_UPPER]  = { ENCODER_CCW_CW(G(KC_MINS), G(KC_EQL)), ENCODER_CCW_CW(KC_TRNS, KC_TRNS) },
    [_ADJUST] = { ENCODER_CCW_CW(HRM_KNOB_DN, HRM_KNOB_UP), ENCODER_CCW_CW(KC_MPRV, KC_MNXT) },
};
#endif
