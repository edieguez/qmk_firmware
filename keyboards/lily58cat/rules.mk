# Real split shared-memory lock on AVR, where QMK's is a no-op (see split_lock.c)
OPT_DEFS += -DPLATFORM_SUPPORTS_SYNCHRONIZATION
SRC += split_lock.c
