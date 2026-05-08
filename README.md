# scroll-o-meter

A one-encoder ZMK keyboard for iPhone. Rotate to scroll, push to read out
the cumulative scroll count, hold to reset.

Built on the working iOS-scroll setup from `agent-keyboard` (branch
`iphone-scroll-encoder`): nice_nano + EC11, BLE Appearance patched to
"HID Generic" so iOS routes pointer reports, AssistiveTouch wake-up burst
on reconnect, legacy 8-bit wheel reports.

## Hardware

- **MCU**: nice_nano v2 (or any pro_micro-pinout BLE board)
- **Input**: one EC11 rotary encoder with push switch
- **Battery**: any single-cell LiPo wired to the nice_nano battery pads

### Wiring

All three GPIO pins are on the **left side** of the Pro Micro footprint
because that side is identical between nice_nano and SuperMini-NRF52840
clones (D0–D9 → P0.06, P0.08, P0.17, P0.20, P0.22, P0.24, P1.00, P0.11,
P1.04, P1.06). Right-side pin maps differ between the two boards.

| Encoder pin | Silkscreen | MCU pin | Position on Pro Micro footprint |
|-------------|------------|---------|------------------|
| A (rotation) | `D2` | P0.17 | left side, 5th pin from top |
| B (rotation) | `D3` | P0.20 | left side, 6th pin from top |
| C (common)   | `GND` | — | any GND pin |
| Switch pin 1 | `D4` | P0.22 | left side, 7th pin from top |
| Switch pin 2 | `GND` | — | any GND pin |

If you change pins, update `boards/shields/scroll_meter/scroll_meter.overlay`.

## Behaviors

| Action | What happens |
|--------|--------------|
| Rotate | iOS scroll (`MOVE_Y(±90)`, ~1 wheel unit per detent) + counter += 1 |
| Tap encoder | Types the current count as ASCII digits into the focused field |
| Hold encoder ≥600ms | Resets counter to 0 and persists |

The counter survives power-off — it's saved to NVS via Zephyr's settings
subsystem every `CONFIG_ZMK_SCROLL_COUNTER_PERSIST_EVERY` detents
(default 25).

## Reading the count on iPhone

1. Pair the keyboard (Settings → Bluetooth → "Scroll Meter").
2. Enable AssistiveTouch (Settings → Accessibility → Touch → AssistiveTouch).
3. Open Notes (or any text field), tap into a text input.
4. Press the encoder. The count types itself in.

## Calibrating to physical distance

The counter records detents, not pixels. To turn it into a "Statue of
Liberty" comparison, calibrate once: scroll a known-length page (e.g.,
a screen with a measured ruler), press to read detents, divide pixels
by detents → constant for your finger speed and the apps you use.

Rough order of magnitude: `~60 px/detent` × 460 dpi iPhone screen ≈ 3.3 mm
of finger-equivalent travel per detent. A Statue-of-Liberty day (~93 m)
is therefore ~28k detents. Heavy but achievable.

## Building

Push to GitHub → CI builds `scroll_meter-nice_nano.uf2` and a
`settings_reset` UF2. Drag the `scroll_meter` UF2 onto the nice_nano in
bootloader mode (double-tap reset).

## Project layout

```
scroll-o-meter/
├── build.yaml                # CI matrix
├── CMakeLists.txt            # picks up src/ when Kconfigs enabled
├── Kconfig                   # iOS nudge + counter options
├── config/west.yml           # ZMK manifest
├── zephyr/module.yml         # Zephyr module manifest
├── boards/shields/scroll_meter/
│   ├── scroll_meter.overlay  # encoder, kscan, behavior nodes
│   ├── scroll_meter.keymap   # bindings (rotate / tap / hold)
│   ├── scroll_meter.conf     # mouse + EC11 + settings + NVS
│   ├── scroll_meter.zmk.yml
│   ├── Kconfig.shield
│   └── Kconfig.defconfig
├── dts/bindings/behaviors/   # custom behavior bindings
└── src/
    ├── ios_pointer_nudge.c   # AssistiveTouch wake-up on reconnect
    ├── scroll_counter.[ch]   # sensor listener + NVS persistence
    ├── behavior_count_print.c
    └── behavior_count_reset.c
```
