# Real split shared-memory lock on AVR, where QMK's is a no-op (see split_lock.c)
OPT_DEFS += -DPLATFORM_SUPPORTS_SYNCHRONIZATION
SRC += split_lock.c

# Size savers for the 28 KB ATmega32u4.
MAGIC_ENABLE = no
SPACE_CADET_ENABLE = no
GRAVE_ESC_ENABLE = no
MOUSEKEY_ENABLE = no

CAPS_WORD_ENABLE = yes

WPM_ENABLE = yes

SRC += lib/bongocat.c
