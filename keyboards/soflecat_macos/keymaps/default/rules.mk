ENCODER_MAP_ENABLE = yes
TAP_DANCE_ENABLE = yes
WPM_ENABLE = yes
CAPS_WORD_ENABLE = yes
KEY_OVERRIDE_ENABLE = yes

# Real controller is a Pro Micro-footprint RP2040 clone, not the AVR Pro
# Micro keyboard.json's development_board targets. `rp2040_ce`, not
# `sparkfun_pm2040`, is the correct target: QMK's own changelog for the
# promicro_rp2040 -> sparkfun_pm2040 rename (commit 56ebd3b829) says
# generic "ProMicro RP2040"-silkscreen clones (this board, per
# soflecat-specs.md) should use rp2040_ce, while sparkfun_pm2040 is for
# genuine SparkFun-branded boards. Behavior-neutral for this keymap either
# way -- the two converters only differ in onboard TX/RX LED pins (unused
# here) and a VBUS-detect pin (moot, since SPLIT_USB_DETECT below already
# makes split_util.c use USB-enumeration polling instead of VBUS sensing).
CONVERT_TO = rp2040_ce

SRC += lib/bongocat.c
