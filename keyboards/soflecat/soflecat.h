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

#pragma once

#include "quantum.h"

// Show a message on the master OLED for a moment, replacing the status
// screen, the way RGB changes are shown. The screen is 5 columns wide;
// oled_show_value() puts `label` over `value`, oled_show_message() takes
// preformatted text with newlines.
#ifdef OLED_ENABLE
void oled_show_value(const char *label, uint16_t value);
void oled_show_message(const char *text);
#else
static inline void oled_show_value(const char *label, uint16_t value) {}
static inline void oled_show_message(const char *text) {}
#endif
