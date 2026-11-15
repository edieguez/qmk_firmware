// Copyright 2026 edieguez
// SPDX-License-Identifier: GPL-2.0-or-later

// Keyboard-level hooks. QMK only compiles this file because its name matches
// the folder; rename both together.
#include <string.h>
#include "quantum.h"
#include "lib/bongocat.h"

#ifdef RGBLIGHT_ENABLE
// rgblight's speed setting doesn't change these effects; each one instead
// comes as a few modes that differ only in speed (and swirl in direction).
// On 6 LEDs that read as duplicates, so Mode+/- jumps between effects and
// Speed+/- steps through an effect's variants.
#    define RGB_EFFECT_COUNT 4
static const uint8_t rgb_effect_base[RGB_EFFECT_COUNT]   = {RGBLIGHT_MODE_STATIC_LIGHT, RGBLIGHT_MODE_BREATHING, RGBLIGHT_MODE_RAINBOW_MOOD, RGBLIGHT_MODE_RAINBOW_SWIRL};
static const uint8_t rgb_effect_speeds[RGB_EFFECT_COUNT] = {1, 4, 3, 3};
static const uint8_t rgb_effect_step[RGB_EFFECT_COUNT]   = {1, 1, 1, 2}; // swirl alternates direction
// Breaks the build if keyboard.json enables an effect this table lacks.
_Static_assert(RGBLIGHT_MODE_last == RGBLIGHT_MODE_RAINBOW_SWIRL + 6, "update rgb_effect_base for the effects in keyboard.json");

static uint8_t rgb_effect(uint8_t mode) {
    uint8_t effect = RGB_EFFECT_COUNT - 1;
    while (effect > 0 && mode < rgb_effect_base[effect]) {
        effect--;
    }
    return effect;
}

// 0 is the slowest.
static uint8_t rgb_effect_speed(uint8_t mode) {
    uint8_t effect = rgb_effect(mode);
    return (mode - rgb_effect_base[effect]) / rgb_effect_step[effect];
}

static void step_rgb_effect(int8_t delta) {
    uint8_t mode   = rgblight_get_mode();
    uint8_t effect = (rgb_effect(mode) + RGB_EFFECT_COUNT + delta) % RGB_EFFECT_COUNT;
    uint8_t speed  = MIN(rgb_effect_speed(mode), rgb_effect_speeds[effect] - 1); // keep the speed across effects
    rgblight_mode(rgb_effect_base[effect] + speed * rgb_effect_step[effect]);
}

// The other half ignores colour updates while its LEDs are off, and turning
// them on only syncs the mode. At power-up the USB half is USB-suspended, so
// its first sync says "off" and the colour is dropped; the other half then
// wakes with its own saved colour. Resend the colour on every off-to-on.
extern rgblight_status_t rgblight_status;

void housekeeping_task_kb(void) {
    static bool was_enabled = false;
    bool        enabled     = rgblight_is_enabled();
    if (is_keyboard_master() && enabled && !was_enabled) {
        rgblight_status.change_flags |= RGBLIGHT_STATUS_CHANGE_HSVS;
    }
    was_enabled = enabled;
    housekeeping_task_user();
}

static void step_rgb_speed(int8_t delta) {
    uint8_t mode   = rgblight_get_mode();
    uint8_t effect = rgb_effect(mode);
    int8_t  speed  = rgb_effect_speed(mode) + delta;
    if (speed < 0 || speed >= rgb_effect_speeds[effect]) {
        return;
    }
    uint8_t direction = (mode - rgb_effect_base[effect]) % rgb_effect_step[effect];
    rgblight_mode(rgb_effect_base[effect] + speed * rgb_effect_step[effect] + direction);
}
#endif

#ifdef OLED_ENABLE

#    ifdef RGBLIGHT_ENABLE
#        define RGB_STATUS_DISPLAY_MS 1500

// Label and value of the last RGB change, shown for a moment instead of the
// status screen. No snprintf: on AVR it pulls in ~1.5 KB of vfprintf.
static const char *rgb_status_label = NULL; // in flash
static char        rgb_status_value[6];     // in RAM
static uint32_t    rgb_status_shown_until = 0;
// Bumped per message. oled_write() only touches the cells it draws, so
// render_status() clears the screen when this changes.
static uint8_t rgb_status_version = 0;

// Sat/val/speed stop changing at their limits, so the diff below misses
// those presses. process_record_kb() flags them.
static bool sat_key_pressed   = false;
static bool val_key_pressed   = false;
static bool speed_key_pressed = false;

// "2/3" style.
static const char *rgb_speed_text(uint8_t mode) {
    static char text[4];
    text[0] = '1' + rgb_effect_speed(mode);
    text[1] = '/';
    text[2] = '0' + rgb_effect_speeds[rgb_effect(mode)];
    return text;
}

static void show_rgb_status(const char *label, const char *value) {
    rgb_status_label = label;
    strncpy(rgb_status_value, value, sizeof(rgb_status_value) - 1);
    rgb_status_shown_until = timer_read32() + RGB_STATUS_DISPLAY_MS;
    rgb_status_version++;
}

// Five characters fit across the portrait screen. Order follows
// rgb_effect_base.
static const char *rgb_mode_name(uint8_t mode) {
    static const char *const names[RGB_EFFECT_COUNT] = {"SOLID", "BRTHE", "MOOD", "SWIRL"};
    return names[rgb_effect(mode)];
}

// Show whichever RGB setting changed last, briefly.
static void poll_rgb_status(void) {
    static bool    initialized  = false;
    static bool    last_enabled = true;
    static uint8_t last_hue     = 0;
    static uint8_t last_sat     = 0;
    static uint8_t last_val     = 0;
    static uint8_t last_mode    = 0;

    bool    enabled = rgblight_is_enabled();
    uint8_t hue     = rgblight_get_hue();
    uint8_t sat     = rgblight_get_sat();
    uint8_t val     = rgblight_get_val();
    uint8_t mode    = rgblight_get_mode();

    // The first call only seeds the baseline, or boot flashes a bogus message.
    if (!initialized) {
        initialized = true;
    } else if (enabled != last_enabled) {
        show_rgb_status(PSTR("RGB"), enabled ? "ON" : "OFF");
    } else if (mode != last_mode && rgb_effect(mode) != rgb_effect(last_mode)) {
        show_rgb_status(PSTR("MODE"), rgb_mode_name(mode));
    } else if (mode != last_mode || speed_key_pressed) {
        show_rgb_status(PSTR("SPEED"), rgb_speed_text(mode));
    } else if (hue != last_hue) {
        show_rgb_status(PSTR("HUE"), get_u8_str(hue, ' '));
    } else if (sat != last_sat || sat_key_pressed) {
        show_rgb_status(PSTR("SAT"), get_u8_str(sat, ' '));
    } else if (val != last_val || val_key_pressed) {
        show_rgb_status(PSTR("BRI"), get_u8_str(val, ' '));
    }

    last_enabled      = enabled;
    last_hue          = hue;
    last_sat          = sat;
    last_val          = val;
    last_mode         = mode;
    sat_key_pressed   = false;
    val_key_pressed   = false;
    speed_key_pressed = false;
}

// False once the message has expired.
static bool rgb_status_active(void) {
    poll_rgb_status();
    return !timer_expired32(timer_read32(), rgb_status_shown_until);
}

#    endif

// Last 20 typed characters, bottom of the master OLED. Only keys with an
// obvious single character are logged. Cleared after 30 s idle so a
// password doesn't linger.
#    define KEYLOG_LEN 20
#    define KEYLOG_CLEAR_MS 30000
static char     keylog[KEYLOG_LEN + 1];
static uint32_t keylog_written_at = 0;

static char keycode_to_char(uint16_t keycode) {
    if (keycode >= KC_A && keycode <= KC_Z) {
        return 'a' + (keycode - KC_A);
    }
    if (keycode >= KC_1 && keycode <= KC_9) {
        return '1' + (keycode - KC_1);
    }
    switch (keycode) {
        case KC_0:
            return '0';
        case KC_SPACE:
            return ' ';
        case KC_MINUS:
            return '-';
        case KC_EQUAL:
            return '=';
        case KC_LBRC:
            return '[';
        case KC_RBRC:
            return ']';
        case KC_BSLS:
            return '\\';
        case KC_SCLN:
            return ';';
        case KC_QUOTE:
            return '\'';
        case KC_COMMA:
            return ',';
        case KC_DOT:
            return '.';
        case KC_SLASH:
            return '/';
        default:
            return 0;
    }
}

static void clear_keylog(void) {
    memset(keylog, ' ', KEYLOG_LEN);
    keylog[KEYLOG_LEN] = '\0';
}

static void update_keylog(uint16_t keycode, const keyrecord_t *record) {
    // Mod-tap and layer-tap keycodes arrive raw. Log the tap keycode, and
    // only when it was a tap.
    if (IS_QK_MOD_TAP(keycode)) {
        if (record->tap.count == 0) {
            return;
        }
        keycode = QK_MOD_TAP_GET_TAP_KEYCODE(keycode);
    } else if (IS_QK_LAYER_TAP(keycode)) {
        if (record->tap.count == 0) {
            return;
        }
        keycode = QK_LAYER_TAP_GET_TAP_KEYCODE(keycode);
    }

    char c = keycode_to_char(keycode);
    if (c == 0) {
        return;
    }
    memmove(keylog, keylog + 1, KEYLOG_LEN - 1);
    keylog[KEYLOG_LEN - 1] = c;
    keylog_written_at      = timer_read32();
}

void keyboard_post_init_kb(void) {
    clear_keylog();
    keyboard_post_init_user();
}

void post_process_record_kb(uint16_t keycode, keyrecord_t *record) {
    if (record->event.pressed) {
        update_keylog(keycode, record);
    }
    post_process_record_user(keycode, record);
}

// Icon glyphs are at 0x80+ in lib/glcdfont.c, taken from
// splitkb/aurora/sofle_v2. The master OLED is portrait, 5 columns x 16 rows.

// Layer numbers follow enum layers in keymap.c.
static void render_layer_icon(uint8_t layer) {
    static const char PROGMEM base_layer[]   = {0x20, 0x94, 0x95, 0x96, 0x20, 0x20, 0xb4, 0xb5, 0xb6, 0x20, 0x20, 0xd4, 0xd5, 0xd6, 0x20, 0};
    static const char PROGMEM lower_layer[]  = {0x20, 0x9a, 0x9b, 0x9c, 0x20, 0x20, 0xba, 0xbb, 0xbc, 0x20, 0x20, 0xda, 0xdb, 0xdc, 0x20, 0};
    static const char PROGMEM upper_layer[]  = {0x20, 0x97, 0x98, 0x99, 0x20, 0x20, 0xb7, 0xb8, 0xb9, 0x20, 0x20, 0xd7, 0xd8, 0xd9, 0x20, 0};
    static const char PROGMEM adjust_layer[] = {0x20, 0x9d, 0x9e, 0x9f, 0x20, 0x20, 0xbd, 0xbe, 0xbf, 0x20, 0x20, 0xdd, 0xde, 0xdf, 0x20, 0};

    switch (layer) {
        case 1:
            oled_write_P(lower_layer, false);
            break;
        case 2:
            oled_write_P(upper_layer, false);
            break;
        case 3:
            oled_write_P(adjust_layer, false);
            break;
        default:
            oled_write_P(base_layer, false);
    }
}

// Fillers between two icons so the pair reads as one shape. Rows 1 and 2.
static const char PROGMEM filler_off_off[] = {0xc5, 0xc6};
static const char PROGMEM filler_on_off[]  = {0xc7, 0xc8};
static const char PROGMEM filler_off_on[]  = {0xc9, 0xca};
static const char PROGMEM filler_on_on[]   = {0xcb, 0xcc};

// Two 2x2 icons with a filler between them. Each icon is four glyphs: top
// left, top right, bottom left, bottom right. The GUI icon means Super.
static void render_icon_pair(const char *left, const char *right, bool left_on, bool right_on) {
    const char *filler = left_on && right_on ? filler_on_on : left_on ? filler_on_off : right_on ? filler_off_on : filler_off_off;
    for (uint8_t row = 0; row < 2; row++) {
        oled_write_char(pgm_read_byte(left + row * 2), false);
        oled_write_char(pgm_read_byte(left + row * 2 + 1), false);
        oled_write_char(pgm_read_byte(filler + row), false);
        oled_write_char(pgm_read_byte(right + row * 2), false);
        oled_write_char(pgm_read_byte(right + row * 2 + 1), false);
    }
}

// `modifiers` is get_mods() | get_speculative_mods().
static void render_mod_icons(uint8_t modifiers) {
    static const char PROGMEM gui_off[]   = {0x85, 0x86, 0xa5, 0xa6};
    static const char PROGMEM gui_on[]    = {0x8d, 0x8e, 0xad, 0xae};
    static const char PROGMEM alt_off[]   = {0x87, 0x88, 0xa7, 0xa8};
    static const char PROGMEM alt_on[]    = {0x8f, 0x90, 0xaf, 0xb0};
    static const char PROGMEM ctrl_off[]  = {0x89, 0x8a, 0xa9, 0xaa};
    static const char PROGMEM ctrl_on[]   = {0x91, 0x92, 0xb1, 0xb2};
    static const char PROGMEM shift_off[] = {0x8b, 0x8c, 0xab, 0xac};
    static const char PROGMEM shift_on[]  = {0xcd, 0xce, 0xcf, 0xd0};

    bool gui   = modifiers & MOD_MASK_GUI;
    bool alt   = modifiers & MOD_MASK_ALT;
    bool ctrl  = modifiers & MOD_MASK_CTRL;
    bool shift = modifiers & MOD_MASK_SHIFT;

    render_icon_pair(gui ? gui_on : gui_off, alt ? alt_on : alt_off, gui, alt);
    render_icon_pair(ctrl ? ctrl_on : ctrl_off, shift ? shift_on : shift_off, ctrl, shift);
}

// Rows: 0-2 layer icon, 4-5 Super/Alt, 6-7 Ctrl/Shift, 9 CAPS, 10 WORD,
// 12-15 keylog. 3, 8 and 11 are spacers.
static void render_status(void) {
#    ifdef RGBLIGHT_ENABLE
    // oled_write() leaves cells it doesn't draw alone, so clear when
    // switching between the RGB message and the status screen, and when the
    // message changes.
    static bool    last_was_rgb_message = false;
    static uint8_t drawn_rgb_version    = 0;
    bool           is_rgb_message       = rgb_status_active();
    if (is_rgb_message != last_was_rgb_message || (is_rgb_message && drawn_rgb_version != rgb_status_version)) {
        oled_clear();
        last_was_rgb_message = is_rgb_message;
        drawn_rgb_version    = rgb_status_version;
    }
    if (is_rgb_message) {
        oled_set_cursor(0, 0);
        oled_write_P(rgb_status_label, false);
        oled_set_cursor(0, 1);
        oled_write(rgb_status_value, false);
        return;
    }
#    endif

    oled_set_cursor(0, 0);
    render_layer_icon(get_highest_layer(layer_state));

    oled_set_cursor(0, 4);
    render_mod_icons(get_mods() | get_speculative_mods());

    led_t led_usb_state = host_keyboard_led_state();
    oled_set_cursor(0, 9);
    oled_write_P(PSTR("CAPS "), led_usb_state.caps_lock);
    oled_set_cursor(0, 10);
    oled_write_P(PSTR("WORD "), is_caps_word_on());

    if (timer_elapsed32(keylog_written_at) > KEYLOG_CLEAR_MS) {
        clear_keylog();
    }
    oled_set_cursor(0, 12);
    oled_write(keylog, false);
}

oled_rotation_t oled_init_kb(oled_rotation_t rotation) {
    if (is_keyboard_master()) {
        return OLED_ROTATION_270; // portrait status screen
    }
    return OLED_ROTATION_180;
}

bool oled_task_kb(void) {
    if (!oled_task_user()) {
        return false;
    }
    if (is_keyboard_master()) {
        render_status();
    } else {
        render_bongocat();
        oled_set_cursor(14, 0);
        oled_write_P(PSTR("WPM:"), false);
        oled_write(get_u8_str(get_current_wpm(), ' '), false);
    }
    return false;
}

#endif

// Runs before rgblight's own key handling, so the effect keys can be taken
// over here.
bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
    if (!process_record_user(keycode, record)) {
        return false;
    }
#ifdef RGBLIGHT_ENABLE
    bool pressed = record->event.pressed;
#    ifdef OLED_ENABLE
    // rgblight consumes UG_* keys, so post_process_record_kb() never sees
    // them. Flag the ones the OLED message can't detect at their limits.
    if (pressed) {
        switch (keycode) {
            case UG_SATU:
            case UG_SATD:
                sat_key_pressed = true;
                break;
            case UG_VALU:
            case UG_VALD:
                val_key_pressed = true;
                break;
            case UG_SPDU:
            case UG_SPDD:
                speed_key_pressed = true;
                break;
        }
    }
#    endif
    switch (keycode) {
        case UG_NEXT:
        case UG_PREV:
            if (pressed) {
                step_rgb_effect(keycode == UG_NEXT ? 1 : -1);
            }
            return false;
        case UG_SPDU:
        case UG_SPDD:
            if (pressed) {
                step_rgb_speed(keycode == UG_SPDU ? 1 : -1);
            }
            return false;
    }
#endif
    return true;
}
