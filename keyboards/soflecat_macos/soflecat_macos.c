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

// Shared size for every status-line buffer below (rgb_status, and
// render_status()'s shown/last_shown) so they can't drift out of sync with
// each other and silently truncate if one gets resized without the others.
#define OLED_STATUS_BUF_LEN 40

#ifdef RGB_MATRIX_ENABLE
#    define RGB_STATUS_DISPLAY_MS 1500

static char     rgb_status[OLED_STATUS_BUF_LEN] = "";
static uint32_t rgb_status_shown_until = 0;

// RM_SATU/RM_SATD, RM_VALU/RM_VALD, and RM_SPDU/RM_SPDD are no-ops once
// their property is already maxed/minned out (they use saturating math,
// unlike hue and flags which wrap around), so the change-based detection
// below never fires for them at the limit. Set by post_process_record_kb()
// (after the keypress has already been applied by process_rgb_matrix),
// consumed and cleared by the next poll below.
static bool sat_key_pressed   = false;
static bool val_key_pressed   = false;
static bool speed_key_pressed = false;

static void show_rgb_status(const char *label, uint8_t value) {
    snprintf(rgb_status, sizeof(rgb_status), "%s\n%u", label, value);
    rgb_status_shown_until = timer_read32() + RGB_STATUS_DISPLAY_MS;
}

// Watches for RGB property changes (toggle/mode/hue/sat/brightness/speed/
// flags), however they were made -- encoder or key -- and records whichever
// one changed most recently into rgb_status, for rgb_status_message() to
// show for a few seconds.
static void poll_rgb_status(void) {
    static bool     initialized  = false;
    static bool     last_enabled = true;
    static uint8_t  last_mode    = 0;
    static uint8_t  last_hue     = 0;
    static uint8_t  last_sat     = 0;
    static uint8_t  last_val     = 0;
    static uint8_t  last_speed   = 0;
    static uint8_t  last_flags   = 0;

    bool    enabled = rgb_matrix_is_enabled();
    uint8_t mode    = rgb_matrix_get_mode();
    uint8_t hue     = rgb_matrix_get_hue();
    uint8_t sat     = rgb_matrix_get_sat();
    uint8_t val     = rgb_matrix_get_val();
    uint8_t speed   = rgb_matrix_get_speed();
    uint8_t flags   = rgb_matrix_get_flags();

    // Seed the baseline from real state on the first poll instead of diffing
    // against the hardcoded initializers above -- otherwise the persisted
    // RGB state almost never matches those defaults and this flashes a
    // bogus status message on every boot.
    if (!initialized) {
        initialized = true;
    } else if (enabled != last_enabled) {
        snprintf(rgb_status, sizeof(rgb_status), "RGB\n%s", enabled ? "ON" : "OFF");
        rgb_status_shown_until = timer_read32() + RGB_STATUS_DISPLAY_MS;
    } else if (mode != last_mode) {
        snprintf(rgb_status, sizeof(rgb_status), "RGB MODE\n%s", rgb_matrix_get_mode_name(mode));
        rgb_status_shown_until = timer_read32() + RGB_STATUS_DISPLAY_MS;
    } else if (hue != last_hue) {
        show_rgb_status("RGB HUE", hue);
    } else if (sat != last_sat || sat_key_pressed) {
        show_rgb_status("RGB SAT", sat);
    } else if (val != last_val || val_key_pressed) {
        show_rgb_status("RGB BRI", val);
    } else if (speed != last_speed || speed_key_pressed) {
        show_rgb_status("RGB SPEED", speed);
    } else if (flags != last_flags) {
        snprintf(rgb_status, sizeof(rgb_status), "RGB FLAGS\n0x%02X", flags);
        rgb_status_shown_until = timer_read32() + RGB_STATUS_DISPLAY_MS;
    }

    last_enabled      = enabled;
    last_mode         = mode;
    last_hue          = hue;
    last_sat          = sat;
    last_val          = val;
    last_speed        = speed;
    last_flags        = flags;
    sat_key_pressed   = false;
    val_key_pressed   = false;
    speed_key_pressed = false;
}

// Returns the current RGB status message while it's still within its
// display window -- or NULL once expired, so the caller falls back to the
// normal layer/caps display.
static const char *rgb_status_message(void) {
    poll_rgb_status();
    if (timer_expired32(timer_read32(), rgb_status_shown_until)) {
        return NULL;
    }
    return rgb_status;
}

#endif

// Rolling log of recently typed characters, shown on the master OLED's last
// row. Only keys with an obvious single-character representation are logged
// (letters, digits, space, and the punctuation keys reachable on _BASE);
// modifiers, layer keys, and navigation/function keys are silently skipped.
// Tap dance's second tap and key-override replacements bypass process_record
// entirely (see the TD_CAPS comment above and quantum/process_key_override.c),
// so neither one reaches this log -- consistent with the rest of the keymap's
// handling of those two features.
#define KEYLOG_LEN 20
static char keylog[KEYLOG_LEN + 1];

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

    // The home row mods' keycode as stored in the keymap is the whole
    // MT(mod, kc) value, not the plain letter -- process_record_kb/
    // post_process_record_kb always see this raw keymap value, regardless of
    // whether the tap-hold system resolved this press as a tap or a hold.
    // record->tap.count is 0 for a hold (only the modifier fired, no
    // character was sent) and nonzero for a tap, so only unwrap and log the
    // letter in the tap case.
    if (IS_QK_MOD_TAP(keycode)) {
        if (record->tap.count == 0) {
            return;
        }
        keycode = QK_MOD_TAP_GET_TAP_KEYCODE(keycode);
    }

    char c = keycode_to_char(keycode);
    if (c == 0) {
        return;
    }
    memmove(keylog, keylog + 1, KEYLOG_LEN - 1);
    keylog[KEYLOG_LEN - 1] = c;
}

void post_process_record_kb(uint16_t keycode, keyrecord_t *record) {
    if (record->event.pressed) {
#ifdef RGB_MATRIX_ENABLE
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
#endif
        update_keylog(keycode, record);
    }
    post_process_record_user(keycode, record);
}

// Icon glyphs (layer-state block, GUI/Alt/Ctrl/Shift mod-status pairs, bleed
// fillers between them) come from lib/glcdfont.c, codepoints 0x80+, reused
// verbatim from keyboards/splitkb/aurora/sofle_v2 -- see that board's
// render_layer_state()/render_mod_status_gui_alt()/render_mod_status_ctrl_shift()
// for the original. Each writes a fixed number of 5-character rows, matching
// this board's master-half portrait grid (OLED_ROTATION_90, 5 cols x 16 rows)
// exactly, so render_status() below can lay them out by row range with no
// extra padding logic.

// 3 rows x 5 cols. Layer numbers match `enum layers` in
// keymaps/default/keymap.c (_BASE, _NAV, _FN, _ADJUST) -- keep in sync if
// layers are added/reordered. Aurora's 4 layer-icon shapes (default/lower/
// raise/adjust) don't have a semantic equivalence to this keymap's layers,
// they're just reused 1:1 in enum order as a visual proxy.
static void render_layer_icon(uint8_t layer) {
    static const char PROGMEM base_layer[] = {0x20, 0x94, 0x95, 0x96, 0x20, 0x20, 0xb4, 0xb5, 0xb6, 0x20, 0x20, 0xd4, 0xd5, 0xd6, 0x20, 0};
    static const char PROGMEM nav_layer[]  = {0x20, 0x9a, 0x9b, 0x9c, 0x20, 0x20, 0xba, 0xbb, 0xbc, 0x20, 0x20, 0xda, 0xdb, 0xdc, 0x20, 0};
    static const char PROGMEM fn_layer[]   = {0x20, 0x97, 0x98, 0x99, 0x20, 0x20, 0xb7, 0xb8, 0xb9, 0x20, 0x20, 0xd7, 0xd8, 0xd9, 0x20, 0};
    static const char PROGMEM adjust_layer[] = {0x20, 0x9d, 0x9e, 0x9f, 0x20, 0x20, 0xbd, 0xbe, 0xbf, 0x20, 0x20, 0xdd, 0xde, 0xdf, 0x20, 0};

    switch (layer) {
        case 1:
            oled_write_P(nav_layer, false);
            break;
        case 2:
            oled_write_P(fn_layer, false);
            break;
        case 3:
            oled_write_P(adjust_layer, false);
            break;
        default:
            oled_write_P(base_layer, false);
    }
}

// 2 rows x 5 cols. `modifiers` should be get_mods()|get_oneshot_mods() so a
// currently-held mod and a one-shot mod both show as "on" -- a currently-
// held mod was previously invisible to this display entirely (only
// get_oneshot_mods() was checked).
static void render_mod_icons_gui_alt(uint8_t modifiers) {
    static const char PROGMEM gui_off_1[] = {0x85, 0x86, 0};
    static const char PROGMEM gui_off_2[] = {0xa5, 0xa6, 0};
    static const char PROGMEM gui_on_1[]  = {0x8d, 0x8e, 0};
    static const char PROGMEM gui_on_2[]  = {0xad, 0xae, 0};

    static const char PROGMEM alt_off_1[] = {0x87, 0x88, 0};
    static const char PROGMEM alt_off_2[] = {0xa7, 0xa8, 0};
    static const char PROGMEM alt_on_1[]  = {0x8f, 0x90, 0};
    static const char PROGMEM alt_on_2[]  = {0xaf, 0xb0, 0};

    // Fillers between the modifier icons bleed into the icon frames, so the
    // pair reads as one continuous shape rather than two disjoint icons.
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

// 2 rows x 5 cols. Same bleed-filler technique as render_mod_icons_gui_alt().
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

// Portrait layout (OLED_ROTATION_270, 5 cols x 16 rows):
//   rows  0-2   layer icon
//   row   3     spacer
//   rows  4-5   GUI | Alt icon pair
//   rows  6-7   Ctrl | Shift icon pair
//   row   8     spacer
//   row   9     "CAPS" bar (full-row invert when caps_lock active)
//   row   10    "WORD" bar (full-row invert when caps word active)
//   row   11    spacer
//   rows  12-15 rolling keylog (20 chars, auto-wraps across exactly 4 rows)
static void render_status(void) {
    const char *rgb_message = NULL;
#ifdef RGB_MATRIX_ENABLE
    rgb_message = rgb_status_message();
#endif

    // oled_write*() only overwrites the character cells it draws into, so
    // switching between an RGB message overlay and the normal status
    // display leaves stale characters behind unless the display is cleared
    // first. Every row below always writes its full fixed-width footprint,
    // so the RGB-message-vs-normal-mode switch is the only real
    // content-shape change left to guard against (no per-row tracking
    // needed).
    static bool last_was_rgb_message = false;
    bool        is_rgb_message       = rgb_message != NULL;
    if (is_rgb_message != last_was_rgb_message) {
        oled_clear();
        last_was_rgb_message = is_rgb_message;
    }

    oled_set_cursor(0, 0);

    if (rgb_message != NULL) {
        oled_write(rgb_message, false);
        return;
    }

    uint8_t mods = get_mods() | get_oneshot_mods();

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

    oled_set_cursor(0, 12);
    oled_write(keylog, false);
}

oled_rotation_t oled_init_kb(oled_rotation_t rotation) {
    if (is_keyboard_master()) {
        // OLED_ROTATION_0 was sideways and OLED_ROTATION_90 came out
        // upside-down for this half's physical mounting -- confirmed by
        // flashing both. 270 is the one that reads right-side up.
        return OLED_ROTATION_270;
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

// If the USB link gets stuck in USB_SUSPENDED without ever receiving a
// genuine bus-reset/resume (e.g. after a full computer restart -- see
// config.h), QMK's suspend loop (chibios.c protocol_pre_task()) blocks
// forever. Recover automatically instead of requiring a physical
// unplug/replug: if we've been continuously suspended longer than this,
// force a warm reboot (mcu_reset() = NVIC_SystemReset(), not the
// bootloader) to force fresh USB re-enumeration. Harmless during a real,
// long sleep -- it just quietly re-suspends again a moment later.
#define STUCK_SUSPEND_TIMEOUT_MS 60000

static bool     suspend_timer_armed = false;
static uint32_t suspend_started_at  = 0;

void suspend_power_down_kb(void) {
    if (!suspend_timer_armed) {
        suspend_timer_armed = true;
        suspend_started_at  = timer_read32();
    } else if (timer_elapsed32(suspend_started_at) > STUCK_SUSPEND_TIMEOUT_MS) {
        mcu_reset();
    }
    suspend_power_down_user();
}

void suspend_wakeup_init_kb(void) {
    suspend_timer_armed = false;
    suspend_wakeup_init_user();
}
