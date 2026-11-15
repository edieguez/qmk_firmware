ENCODER_MAP_ENABLE = yes
TAP_DANCE_ENABLE = yes
WPM_ENABLE = yes
CAPS_WORD_ENABLE = yes
KEY_OVERRIDE_ENABLE = yes

# Pro Micro footprint RP2040 clone. rp2040_ce is the target for generic
# clones; sparkfun_pm2040 is for real SparkFun boards.
CONVERT_TO = rp2040_ce

SRC += lib/bongocat.c
