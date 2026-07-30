ENCODER_MAP_ENABLE = yes
TAP_DANCE_ENABLE = yes
WPM_ENABLE = yes
CAPS_WORD_ENABLE = yes
KEY_OVERRIDE_ENABLE = yes

# Real controller is a Pro Micro-footprint RP2040 clone, not the AVR Pro
# Micro keyboard.json's development_board targets. `sparkfun_pm2040` is what
# the older deprecated `promicro_rp2040` option aliased to internally, so this
# is a like-for-like swap, not a hardware-behavior change.
CONVERT_TO = sparkfun_pm2040

SRC += lib/bongocat.c
