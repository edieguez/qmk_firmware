// Copyright 2026 edieguez
// SPDX-License-Identifier: GPL-2.0-or-later

#include "synchronization_util.h"
#include <avr/io.h>
#include <avr/interrupt.h>

#if defined(SPLIT_KEYBOARD)
// The serial target ISR writes the split shared memory, so a handler that
// copies it and then clears a flag must not be interrupted in between.
static uint8_t split_shared_memory_sreg;

void split_shared_memory_lock(void) {
    uint8_t sreg = SREG;
    cli();
    split_shared_memory_sreg = sreg;
}

void split_shared_memory_unlock(void) {
    __asm__ __volatile__("" ::: "memory");
    SREG = split_shared_memory_sreg;
}
#endif
