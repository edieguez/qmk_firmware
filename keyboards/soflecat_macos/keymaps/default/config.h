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

// Stock font plus the layer and modifier icons at 0x80+.
#ifdef OLED_ENABLE
#    define OLED_FONT_H "keyboards/soflecat_macos/keymaps/default/lib/glcdfont.c"
#endif

// The half with USB traffic is the master. Helps ProMicros where the slave
// doesn't boot.
#define SPLIT_USB_DETECT
// With a power-only cable both halves decide they're the slave and stay
// that way. This reboots a slave that never hears from a master.
#define SPLIT_WATCHDOG_ENABLE

#define SPLIT_WPM_ENABLE         // bongocat speed on the slave
#define SPLIT_ACTIVITY_ENABLE    // keeps the slave OLED awake while typing on the master
#define SPLIT_OLED_ENABLE        // both OLEDs sleep and wake together
#define SPLIT_LAYER_STATE_ENABLE // the slave lights its own LEDs for the layer overlay
#define RGB_MATRIX_KEYPRESSES    // reactive effects
#define RGB_MATRIX_FRAMEBUFFER_EFFECTS // typing heatmap
#define SPLIT_TRANSPORT_MIRROR   // so the slave sees the master's presses for the reactive effects
#define RGB_MATRIX_SOLID_REACTIVE_GRADIENT_MODE // reactive flash hue drifts over time
#define RGB_MATRIX_MODE_NAME_ENABLE // mode name on the OLED
#define RGB_MATRIX_SLEEP
#define OLED_FADE_OUT
#define OLED_FADE_OUT_INTERVAL 5
#define RGB_MATRIX_MAXIMUM_BRIGHTNESS 100 // higher can brown out the controller

// Home row mods. See keymap.c for the per-key exceptions.
#define TAPPING_TERM 200
#define PERMISSIVE_HOLD
#define PERMISSIVE_HOLD_PER_KEY
#define CHORDAL_HOLD
#define FLOW_TAP_TERM 150
#define TAPPING_TERM_PER_KEY
// Send the modifier on keydown and take it back if the key turns out to be
// a tap. Makes Shift/Cmd+click instant. Off within 150 ms of the previous
// key so fast typing doesn't flash lone modifiers.
#define SPECULATIVE_HOLD
#define SPECULATIVE_HOLD_FLOW_TERM 150
