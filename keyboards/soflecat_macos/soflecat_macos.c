/* Copyright 2022 Brian Low
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

#include <string.h>
#include "quantum.h"
#include "lib/bongocat.h"

#ifdef OLED_ENABLE

#define OLED_STATUS_BUF_LEN 40

#ifdef RGB_MATRIX_ENABLE
#    define RGB_STATUS_DISPLAY_MS 1500

static char     rgb_status[OLED_STATUS_BUF_LEN] = "";
static uint32_t rgb_status_shown_until = 0;
// Bumped per message. oled_write() only touches the cells it draws, so
// render_status() clears the screen when this changes.
static uint8_t  rgb_status_version = 0;

// Sat/val/speed stop changing at their limits, so the diff below misses
// those presses. process_record_kb() flags them.
static bool sat_key_pressed   = false;
static bool val_key_pressed   = false;
static bool speed_key_pressed = false;

static void show_rgb_status(const char *label, uint8_t value) {
    snprintf(rgb_status, sizeof(rgb_status), "%s\n%u", label, value);
    rgb_status_shown_until = timer_read32() + RGB_STATUS_DISPLAY_MS;
    rgb_status_version++;
}

// Show whichever RGB setting changed last, briefly, whether by key or knob.
static void poll_rgb_status(void) {
    static bool     initialized  = false;
    static bool     last_enabled = true;
    static uint8_t  last_mode    = 0;
    static uint8_t  last_hue     = 0;
    static uint8_t  last_sat     = 0;
    static uint8_t  last_val     = 0;
    static uint8_t  last_speed   = 0;

    bool    enabled = rgb_matrix_is_enabled();
    uint8_t mode    = rgb_matrix_get_mode();
    uint8_t hue     = rgb_matrix_get_hue();
    uint8_t sat     = rgb_matrix_get_sat();
    uint8_t val     = rgb_matrix_get_val();
    uint8_t speed   = rgb_matrix_get_speed();

    // The first call only seeds the baseline, or boot flashes a bogus message.
    if (!initialized) {
        initialized = true;
    } else if (enabled != last_enabled) {
        snprintf(rgb_status, sizeof(rgb_status), "RGB\n%s", enabled ? "ON" : "OFF");
        rgb_status_shown_until = timer_read32() + RGB_STATUS_DISPLAY_MS;
        rgb_status_version++;
    } else if (mode != last_mode) {
        snprintf(rgb_status, sizeof(rgb_status), "RGB MODE\n%s", rgb_matrix_get_mode_name(mode));
        rgb_status_shown_until = timer_read32() + RGB_STATUS_DISPLAY_MS;
        rgb_status_version++;
    } else if (hue != last_hue) {
        show_rgb_status("RGB HUE", hue);
    } else if (sat != last_sat || sat_key_pressed) {
        show_rgb_status("RGB SAT", sat);
    } else if (val != last_val || val_key_pressed) {
        show_rgb_status("RGB BRI", val);
    } else if (speed != last_speed || speed_key_pressed) {
        show_rgb_status("RGB SPEED", speed);
    }

    last_enabled      = enabled;
    last_mode         = mode;
    last_hue          = hue;
    last_sat          = sat;
    last_val          = val;
    last_speed        = speed;
    sat_key_pressed   = false;
    val_key_pressed   = false;
    speed_key_pressed = false;
}

// NULL once the message has expired.
static const char *rgb_status_message(void) {
    poll_rgb_status();
    if (timer_expired32(timer_read32(), rgb_status_shown_until)) {
        return NULL;
    }
    return rgb_status;
}

#endif

// Last 20 typed characters, bottom of the master OLED. Only keys with an
// obvious single character are logged. Cleared after 30 s idle so a
// password doesn't linger.
#define KEYLOG_LEN 20
#define KEYLOG_CLEAR_MS 30000
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

static void update_keylog(uint16_t keycode, const keyrecord_t *record) {
    static bool initialized = false;
    if (!initialized) {
        memset(keylog, ' ', KEYLOG_LEN);
        keylog[KEYLOG_LEN] = '\0';
        initialized         = true;
    }

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

// process_rgb_matrix() consumes RM_* keys, so post_process_record_kb()
// never sees them.
bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
#ifdef RGB_MATRIX_ENABLE
    if (record->event.pressed) {
        switch (keycode) {
            case RM_SATU:
            case RM_SATD:
                sat_key_pressed = true;
                break;
            case RM_VALU:
            case RM_VALD:
                val_key_pressed = true;
                break;
            case RM_SPDU:
            case RM_SPDD:
                speed_key_pressed = true;
                break;
        }
    }
#endif
    return process_record_user(keycode, record);
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

// `modifiers` is get_mods() | get_oneshot_mods() | get_speculative_mods().
static void render_mod_icons_gui_alt(uint8_t modifiers) {
    static const char PROGMEM gui_off_1[] = {0x85, 0x86, 0};
    static const char PROGMEM gui_off_2[] = {0xa5, 0xa6, 0};
    static const char PROGMEM gui_on_1[]  = {0x8d, 0x8e, 0};
    static const char PROGMEM gui_on_2[]  = {0xad, 0xae, 0};

    static const char PROGMEM alt_off_1[] = {0x87, 0x88, 0};
    static const char PROGMEM alt_off_2[] = {0xa7, 0xa8, 0};
    static const char PROGMEM alt_on_1[]  = {0x8f, 0x90, 0};
    static const char PROGMEM alt_on_2[]  = {0xaf, 0xb0, 0};

    // Fillers between the two icons so the pair reads as one shape.
    static const char PROGMEM off_off_1[] = {0xc5, 0};
    static const char PROGMEM off_off_2[] = {0xc6, 0};
    static const char PROGMEM on_off_1[]  = {0xc7, 0};
    static const char PROGMEM on_off_2[]  = {0xc8, 0};
    static const char PROGMEM off_on_1[]  = {0xc9, 0};
    static const char PROGMEM off_on_2[]  = {0xca, 0};
    static const char PROGMEM on_on_1[]   = {0xcb, 0};
    static const char PROGMEM on_on_2[]   = {0xcc, 0};

    bool gui = modifiers & MOD_MASK_GUI;
    bool alt = modifiers & MOD_MASK_ALT;

    oled_write_P(gui ? gui_on_1 : gui_off_1, false);
    oled_write_P(gui && alt ? on_on_1 : gui ? on_off_1 : alt ? off_on_1 : off_off_1, false);
    oled_write_P(alt ? alt_on_1 : alt_off_1, false);

    oled_write_P(gui ? gui_on_2 : gui_off_2, false);
    oled_write_P(gui && alt ? on_on_2 : gui ? on_off_2 : alt ? off_on_2 : off_off_2, false);
    oled_write_P(alt ? alt_on_2 : alt_off_2, false);
}

static void render_mod_icons_ctrl_shift(uint8_t modifiers) {
    static const char PROGMEM ctrl_off_1[] = {0x89, 0x8a, 0};
    static const char PROGMEM ctrl_off_2[] = {0xa9, 0xaa, 0};
    static const char PROGMEM ctrl_on_1[]  = {0x91, 0x92, 0};
    static const char PROGMEM ctrl_on_2[]  = {0xb1, 0xb2, 0};

    static const char PROGMEM shift_off_1[] = {0x8b, 0x8c, 0};
    static const char PROGMEM shift_off_2[] = {0xab, 0xac, 0};
    static const char PROGMEM shift_on_1[]  = {0xcd, 0xce, 0};
    static const char PROGMEM shift_on_2[]  = {0xcf, 0xd0, 0};

    static const char PROGMEM off_off_1[] = {0xc5, 0};
    static const char PROGMEM off_off_2[] = {0xc6, 0};
    static const char PROGMEM on_off_1[]  = {0xc7, 0};
    static const char PROGMEM on_off_2[]  = {0xc8, 0};
    static const char PROGMEM off_on_1[]  = {0xc9, 0};
    static const char PROGMEM off_on_2[]  = {0xca, 0};
    static const char PROGMEM on_on_1[]   = {0xcb, 0};
    static const char PROGMEM on_on_2[]   = {0xcc, 0};

    bool ctrl  = modifiers & MOD_MASK_CTRL;
    bool shift = modifiers & MOD_MASK_SHIFT;

    oled_write_P(ctrl ? ctrl_on_1 : ctrl_off_1, false);
    oled_write_P(ctrl && shift ? on_on_1 : ctrl ? on_off_1 : shift ? off_on_1 : off_off_1, false);
    oled_write_P(shift ? shift_on_1 : shift_off_1, false);

    oled_write_P(ctrl ? ctrl_on_2 : ctrl_off_2, false);
    oled_write_P(ctrl && shift ? on_on_2 : ctrl ? on_off_2 : shift ? off_on_2 : off_off_2, false);
    oled_write_P(shift ? shift_on_2 : shift_off_2, false);
}

// Rows: 0-2 layer icon, 4-5 GUI/Alt, 6-7 Ctrl/Shift, 9 CAPS, 10 WORD,
// 12-15 keylog. 3, 8 and 11 are spacers.
static void render_status(void) {
    const char *rgb_message = NULL;
#ifdef RGB_MATRIX_ENABLE
    rgb_message = rgb_status_message();
#endif

    // oled_write() leaves cells it doesn't draw alone, so clear when switching
    // between the RGB message and the status screen, and when the message
    // text changes.
    static bool    last_was_rgb_message = false;
    static uint8_t drawn_rgb_version    = 0;
    bool           is_rgb_message       = rgb_message != NULL;
    if (is_rgb_message != last_was_rgb_message) {
        oled_clear();
        last_was_rgb_message = is_rgb_message;
    }

    oled_set_cursor(0, 0);

    if (rgb_message != NULL) {
#ifdef RGB_MATRIX_ENABLE
        if (drawn_rgb_version != rgb_status_version) {
            oled_clear();
            drawn_rgb_version = rgb_status_version;
        }
#endif
        oled_write(rgb_message, false);
        return;
    }

    uint8_t mods = get_mods() | get_oneshot_mods() | get_speculative_mods();

    render_layer_icon(get_highest_layer(layer_state));

    oled_set_cursor(0, 3);
    oled_write_P(PSTR("     "), false);

    oled_set_cursor(0, 4);
    render_mod_icons_gui_alt(mods);
    render_mod_icons_ctrl_shift(mods);

    oled_set_cursor(0, 8);
    oled_write_P(PSTR("     "), false);

    led_t led_usb_state = host_keyboard_led_state();
    oled_set_cursor(0, 9);
    oled_write_P(PSTR("CAPS "), led_usb_state.caps_lock);

    oled_set_cursor(0, 10);
    oled_write_P(PSTR("WORD "), is_caps_word_on());

    oled_set_cursor(0, 11);
    oled_write_P(PSTR("     "), false);

    if (timer_elapsed32(keylog_written_at) > KEYLOG_CLEAR_MS) {
        memset(keylog, ' ', KEYLOG_LEN);
    }
    oled_set_cursor(0, 12);
    oled_write(keylog, false);
}

oled_rotation_t oled_init_kb(oled_rotation_t rotation) {
    if (is_keyboard_master()) {
        return OLED_ROTATION_270; // right side up for how the master screen is mounted
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
        char buf[8];
        snprintf(buf, sizeof(buf), "WPM:%3d", get_current_wpm());
        oled_set_cursor(14, 0);
        oled_write(buf, false);
    }

    return false;
}

#endif

// If USB stays suspended with no bus reset (it happens after a host
// restart), QMK's suspend loop never exits and only a replug helps. After a
// minute of suspend, reset the MCU once so it re-enumerates. The token lives
// in RAM that isn't cleared on reset, so a long real sleep doesn't reset
// every minute; a real wake clears it.
#define STUCK_SUSPEND_TIMEOUT_MS 60000
#define STUCK_SUSPEND_RECOVERED_TOKEN 0x5C0FFEE5

static volatile uint32_t __attribute__((section(".ram0.stuck_suspend_recovered"))) stuck_suspend_recovered;

static bool     suspend_timer_armed = false;
static uint32_t suspend_started_at  = 0;

void suspend_power_down_kb(void) {
    if (!suspend_timer_armed) {
        suspend_timer_armed = true;
        suspend_started_at  = timer_read32();
    } else if (stuck_suspend_recovered != STUCK_SUSPEND_RECOVERED_TOKEN && timer_elapsed32(suspend_started_at) > STUCK_SUSPEND_TIMEOUT_MS) {
        stuck_suspend_recovered = STUCK_SUSPEND_RECOVERED_TOKEN;
        mcu_reset();
    }
    suspend_power_down_user();
}

void suspend_wakeup_init_kb(void) {
    suspend_timer_armed     = false;
    stuck_suspend_recovered = 0;
    suspend_wakeup_init_user();
}
