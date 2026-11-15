// Copyright 2026 edieguez
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

// Home row mods. See keymap.c for the per-key exceptions.
#define TAPPING_TERM 200
#define TAPPING_TERM_PER_KEY
#define PERMISSIVE_HOLD
#define PERMISSIVE_HOLD_PER_KEY
#define CHORDAL_HOLD
#define FLOW_TAP_TERM 150
// Send the modifier on keydown and take it back if the key turns out to be
// a tap. Makes Shift/Ctrl+click instant. Off within 150 ms of the previous
// key so fast typing doesn't flash lone modifiers.
#define SPECULATIVE_HOLD
#define SPECULATIVE_HOLD_FLOW_TERM 150

// Stock font plus the layer and modifier icons at 0x80+.
#define OLED_FONT_H "keyboards/lily58cat/lib/glcdfont.c"
#define SPLIT_ACTIVITY_ENABLE // keeps the slave OLED awake while typing on the master
#define SPLIT_OLED_ENABLE     // both OLEDs sleep and wake together
#define SPLIT_WPM_ENABLE      // bongocat speed on the slave
#define OLED_FADE_OUT
#define OLED_FADE_OUT_INTERVAL 5
