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

#ifdef RGB_MATRIX_ENABLE
#    define RGB_STATUS_DISPLAY_MS 1500

static char     rgb_status[40]         = "";
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

void post_process_record_kb(uint16_t keycode, keyrecord_t *record) {
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
    post_process_record_user(keycode, record);
}
#endif

// Layer numbers match `enum layers` in keymaps/default/keymap.c
// (_BASE, _NAV, _FN, _ADJUST) -- keep in sync if layers are added/reordered.
static void render_status(void) {
    const char *rgb_message = NULL;
#ifdef RGB_MATRIX_ENABLE
    rgb_message = rgb_status_message();
#endif

    // oled_write*() only overwrites the character cells it draws into, so
    // switching between differently-shaped content (RGB message <-> layer
    // status, or one RGB message <-> another) leaves stale characters
    // behind unless the display is cleared first. Only clear on an actual
    // content-shape change, not every tick, to avoid flicker.
    static char last_shown[40] = "";
    char        shown[40];
    if (rgb_message != NULL) {
        snprintf(shown, sizeof(shown), "%s", rgb_message);
    } else {
        snprintf(shown, sizeof(shown), "layer%u", get_highest_layer(layer_state));
    }
    if (strcmp(shown, last_shown) != 0) {
        oled_clear();
        strcpy(last_shown, shown);
    }
    oled_set_cursor(0, 0);

    if (rgb_message != NULL) {
        oled_write(rgb_message, false);
        return;
    }

    oled_write_P(PSTR("LAYER\n"), false);
    switch (get_highest_layer(layer_state)) {
        case 0:
            oled_write_ln_P(PSTR("Base"), false);
            break;
        case 1:
            oled_write_ln_P(PSTR("Nav"), false);
            break;
        case 2:
            oled_write_ln_P(PSTR("Fn"), false);
            break;
        case 3:
            oled_write_ln_P(PSTR("Adjust"), false);
            break;
        default:
            oled_write_ln_P(PSTR("Undef"), false);
    }

    led_t led_usb_state = host_keyboard_led_state();
    oled_write_P(PSTR("CAPS "), led_usb_state.caps_lock);
    oled_write_ln_P(PSTR("CW"), is_caps_word_on());
    oled_write_ln_P(PSTR("OSM"), get_oneshot_mods() != 0);
}

oled_rotation_t oled_init_kb(oled_rotation_t rotation) {
    if (is_keyboard_master()) {
        return rotation;
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
