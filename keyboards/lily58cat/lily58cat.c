// Copyright 2026 edieguez
// SPDX-License-Identifier: GPL-2.0-or-later

// Keyboard-level hooks. QMK only compiles this file because its name matches
// the folder; rename both together.
#include "quantum.h"

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
