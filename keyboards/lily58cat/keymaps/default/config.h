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

