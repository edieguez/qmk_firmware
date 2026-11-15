# lily58cat

A personal, Linux-focused keymap for a stock [Lily58](https://github.com/kata0510/Lily58)
(ATmega32u4 Pro Micro, Caterina bootloader, underglow, 128x32 OLEDs), ported from
`keyboards/soflecat`.

* Keyboard maintainer: [liliums](https://github.com/kata0510) (hardware)
* Hardware supported: Lily58 rev1 with Pro Micro

Make example for this keyboard (after setting up your build environment):

    qmk compile -kb lily58cat -km default

Flashing example (press the reset button on the half when avrdude waits for the port; repeat for the other half):

    qmk flash -kb lily58cat -km default

See the [build environment setup](https://docs.qmk.fm/#/getting_started_build_tools) and the [make instructions](https://docs.qmk.fm/#/getting_started_make_guide) for more information. Brand new to QMK? Start with our [Complete Newbies Guide](https://docs.qmk.fm/#/newbs).

## Bootloader

* **Physical reset button**: press the reset button on the half's PCB (double-tap on some Pro Micro clones)
* **Bootmagic reset**: hold the top-left key (Esc) on the left half while plugging it in
