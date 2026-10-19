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
#pragma once

// Custom font adds icon glyphs (layer state, GUI/Alt/Ctrl/Shift mod status)
// at codepoints 0x80+ on top of the stock glyph table, for render_status()'s
// icon-based master-half OLED display -- see lib/glcdfont.c.
#ifdef OLED_ENABLE
#    define OLED_FONT_H "keyboards/soflecat_macos/keymaps/default/lib/glcdfont.c"
#endif

// Enabling this option changes the startup behavior to listen for an
// active USB communication to delegate which part is master and which
// is slave. With this option enabled and there's USB communication,
// then that half assumes it is the master, otherwise it assumes it
// is the slave.
//
// I've found this helps with some ProMicros where the slave does not boot
#define SPLIT_USB_DETECT

// SPLIT_USB_DETECT only knows "master" once USB fully enumerates -- a
// power-only cable (no data lines) never gets there, so both halves time
// out and both decide they're the slave, permanently (bongocat on both
// OLEDs, no keys/RGB) until a manual power cycle. This reboots any half
// stuck as "slave" that hasn't heard from a master, so it retries
// detection on its own once a real data connection shows up.
#define SPLIT_WATCHDOG_ENABLE

#define SPLIT_WPM_ENABLE     // sync WPM from master to the slave half for the bongocat OLED
#define SPLIT_ACTIVITY_ENABLE // sync matrix activity timestamps so the slave's OLED (bongocat) doesn't sleep mid-typing when it's only seeing keypresses from the other half
#define SPLIT_OLED_ENABLE // mirror the master's actual on/off state to the slave, so both OLEDs blank/wake together instead of each half computing its own timeout independently
#define RGB_MATRIX_MODE_NAME_ENABLE // enables rgb_matrix_get_mode_name() for the OLED status message
#define RGB_MATRIX_SLEEP     // turn off effects when suspended
#define SPLIT_TRANSPORT_MIRROR             // If LED_MATRIX_KEYPRESSES or LED_MATRIX_KEYRELEASES is enabled, you also will want to enable SPLIT_TRANSPORT_MIRROR
#define RGB_MATRIX_MAXIMUM_BRIGHTNESS 100  // limits maximum brightness of LEDs (max 255). Higher may cause the controller to crash.

// Home row mods (A/S/D/F, J/K/L/;): reduce accidental modifier triggers
// during fast/rolled typing. See docs/tap_hold.md "Configuring Home Row Mods".
#define TAPPING_TERM 200
#define PERMISSIVE_HOLD
#define CHORDAL_HOLD
#define FLOW_TAP_TERM 150
// Required for get_tapping_term() in keymap.c to actually be consulted --
// see docs/tap_hold.md "Per Key Tapping Term".
#define TAPPING_TERM_PER_KEY
// Required for get_hold_on_other_key_press() in keymap.c to actually be
// consulted -- see docs/tap_hold.md "Hold On Other Key Press". Scoped to
// the Shift/GUI home row mods only (not a blanket HOLD_ON_OTHER_KEY_PRESS),
// since it's specifically fixing fast-typing chord detection for those four.
#define HOLD_ON_OTHER_KEY_PRESS_PER_KEY

// Defaults to 0 (quantum/action.h) -- the wait_ms(TAP_CODE_DELAY) in
// keymap.c's caps_dance_reset() was a no-op without this, which is why the
// double-tap-to-Caps-Lock tap dance didn't actually fix anything until now.
// 100ms matches the value docs/config_options.md itself suggests for "issues
// with it registering properly".
#define TAP_CODE_DELAY 100
