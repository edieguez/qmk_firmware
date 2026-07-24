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

#include "quantum.h"
#include "lib/bongocat.h"

#ifdef OLED_ENABLE

// Layer numbers match `enum layers` in keymaps/default/keymap.c
// (_BASE, _NAV, _FN, _ADJUST) -- keep in sync if layers are added/reordered.
static void render_status(void) {
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

    oled_write_P(PSTR("\n"), false);
    led_t led_usb_state = host_keyboard_led_state();
    oled_write_ln_P(PSTR("CPSLK"), led_usb_state.caps_lock);
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
