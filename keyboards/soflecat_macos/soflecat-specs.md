# Sofle Choc Keyboard — Hardware & Firmware Reference

## Hardware

- **Keyboard**: Sofle Choc — open-source split keyboard design by Brian Low (merged into QMK as PR #16736).
- **Switches**: Low-profile Kailh Choc.
- **Source**: Purchased pre-assembled from an Etsy seller.
- **Controller**: Generic "ProMicro RP2040" clone board (confirmed by silkscreen text on the board itself), plugged into Pro Micro-footprint sockets on each half.
- **MCU**: RP2040 (ChibiOS platform in QMK), uses the RP2040 UF2 bootloader (ROM-resident, effectively unbrickable).
  - Enter bootloader: double-tap the reset button on a half. It mounts as a USB mass-storage drive named `RPI-RP2`.
  - Flashing: drag-and-drop / copy the `.uf2` file onto that drive; the board reboots automatically into firmware.
- **USB VID/PID**: `0x424C` / `0x5343` (identified via `lsusb`, matches `keyboard.json`).
- **Lighting**: WS2812 (SK6812 Mini-E) per-key RGB LEDs. `RGBLED_NUM 70`, split `{35,35}` (35 LEDs per half).
- **Displays**: OLED screen on each half (I2C).
- **Encoders**: One rotary encoder per half (pin_a F5 / pin_b F4 in the AVR-pinout keyboard.json, translated via the converter for RP2040).
- **Split link**: TRRS cable between halves.

## Firmware

- Using the **Vial** fork of QMK: `vial-kb/vial-qmk`, `vial` branch.
- Cloned locally at `/home/$USER/qmk_firmware/` via:

  ```shell
  qmk setup vial-kb/vial-qmk -b vial -y
  ```

  This is QMK_HOME for all builds (~1.4GB with submodules: chibios, chibios-contrib, pico-sdk, lvgl, etc.).

- QMK keyboard directory: `keyboards/sofle_choc/`
- Since the physical controller is an RP2040 Pro Micro clone but the keyboard definition (`keyboard.json`) is written for AVR Pro Micro pinout, the build uses QMK's **converter** feature:

  ```shell
  -e CONVERT_TO=promicro_rp2040
  ```

### Keymap used: `vial_full`

Located at `keyboards/sofle_choc/keymaps/vial_full/` (created by copying the stock `vial` keymap and fixing feature flags — see "Known issue" below).

`rules.mk` (final, working version):

```properties
OLED_ENABLE = yes
ENCODER_ENABLE = yes
CONSOLE_ENABLE = no
EXTRAKEY_ENABLE = no
VIA_ENABLE = yes
VIAL_ENABLE = yes
ENCODER_MAP_ENABLE = yes
LTO_ENABLE = no
BACKLIGHT_ENABLE = no
RGBLIGHT_ENABLE = no
VIALRGB_ENABLE = yes
QMK_SETTINGS = no
MOUSEKEY_ENABLE = no
COMBO_ENABLE = no
KEY_OVERRIDE_ENABLE = no
RGB_MATRIX_ENABLE = yes
```

Key `config.h` settings (`keymaps/vial_full/config.h`, inherited from `vial` keymap):

- `RP2040_BOOTLOADER_DOUBLE_TAP_RESET`
- `SPLIT_USB_DETECT`
- `SPLIT_TRANSPORT_MIRROR`
- `RGB_DISABLE_WHEN_USB_SUSPENDED`
- `RGB_MATRIX_MAXIMUM_BRIGHTNESS 100` (comment in source: "Higher may cause the controller to crash.")
- `RGBLED_NUM 70`, `RGBLED_SPLIT {35,35}`
- `RGB_MATRIX_FRAMEBUFFER_EFFECTS`, `RGB_MATRIX_KEYPRESSES`
- `VIAL_KEYBOARD_UID {0xA7, 0xA7, 0x9C, 0x4C, 0xE0, 0xB8, 0x00, 0x8D}`
- `VIAL_UNLOCK_COMBO_ROWS {0,5,3,8}` / `VIAL_UNLOCK_COMBO_COLS {0,0,0,0}`

### Build command

```shell
qmk compile -kb sofle_choc -km vial_full -e CONVERT_TO=promicro_rp2040
```

Output file: `sofle_choc_vial_full_promicro_rp2040.uf2` (in `/home/$USER/qmk_firmware/`).

### Flash command (run for EACH half, one at a time)

1. Double-tap reset on the half to put it into bootloader mode (drive `RPI-RP2` appears, usually under `/run/media/<user>/RPI-RP2/` on Linux).
2. Copy the firmware:

   ```shell
   cp /home/$USER/qmk_firmware/sofle_choc_vial_full_promicro_rp2040.uf2 /run/media/$USER/RPI-RP2/
   ```

3. Repeat for the other half.

> Note: the user prefers to perform the actual flashing themselves — the assistant's role is to compile and hand over the exact command, not to copy files onto the bootloader drive directly.
