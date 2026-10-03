# Plants vs. Zombies BeagleBone Practice Project

## Hardware setup

The display and touch controller share SPI0 on the BeagleBone Black:

- P9_3: VCC
- P9_1: GND
- P9_17: CS0
- P9_24: Reset
- P9_23: DC
- P9_18: MOSI (SDI, T_DIN)
- P9_22: SCLK
- P9_3: LED (3.3V)
- P9_21: MISO (SDO, T_DO)
- P8_10: CS1
- P8_9: IRQ

## User-space architecture

The application in `src/` uses one input worker and one owning game thread:

```text
/dev/touch -> input worker -> 16-entry touch queue
                                  |
                                  v
                         main/UI controller
                                  |
                                  v
                         opaque game state
                                  |
                                  v
                         render-only snapshot
                                  |
                                  v
                    renderer -> framebuffer -> /dev/lcd
```

Only the input worker accesses the touch descriptor. The main thread is the
only writer of game state and the only thread that renders or presents frames.
Game updates use a fixed 100 ms step with at most four catch-up updates per
frame. Plants, zombies, and bullets use fixed-capacity pools; individual game
entities are never heap allocated.

Zombie pressure escalates indefinitely by spawn event: four events spawn one
zombie each, the next four spawn two each, the next four spawn three each, and
so on. Each event is still limited by the fixed 100-zombie pool. When a zombie
reaches the house, the simulation pauses and displays a touchable PLAY AGAIN
dialog. YES resets all game state and the wave counter; NO exits cleanly.

## Build and test

From `practice_project/src`:

```sh
make check
make sanitize
make app
```

- `make check` builds and runs warning-as-error host tests.
- `make sanitize` runs the same tests with AddressSanitizer and
  UndefinedBehaviorSanitizer.
- `make app` uses `arm-linux-gnueabihf-gcc` by default and creates the
  static target binary at `build/target/pvz`.
- Override `CROSS_COMPILE` or use `STATIC=0` when a different toolchain or
  dynamic target build is required.
- `make clean` removes only the `src/build` directory.

## Device ABI contract

### Touch input

The user-space adapter opens `/dev/touch` read-only, waits for `POLLIN`, and
requests exactly four bytes. The byte layout is:

```text
bytes 0..1: raw Y ADC value
bytes 2..3: raw X ADC value
```

Both values are right-shifted by three, calibrated, clamped, and scaled to the
320x240 screen. A correct driver returns `4`. The current driver copies the
sample but returns `0`; user space accepts that legacy result only after a
successful `POLLIN` and prints one warning. Any other short result is an
error.

### Display output

The adapter opens `/dev/lcd` write-only and submits exactly one 153,600-byte
320x240 frame per call. Pixels are two-byte, big-endian RGB555 values.
Generated sprite bytes are copied as stored, and `0xFFFF` is the transparent
sprite key.

A short display write is fatal. User space must not continue a partial frame
with another write because the current driver resets the LCD window at the
start of each call.

## Kernel follow-ups

The kernel module is intentionally outside this user-space refactor. A future
driver change should:

- make touch `read()` validate the requested size, propagate SPI failures,
  and return four after a successful copy;
- make LCD `write()` send the actual final chunk length instead of always
  sending 800 bytes;
- avoid unsigned remaining-length underflow for non-800-byte-aligned writes;
- check and propagate all `spi_write()` and `spi_write_then_read()` errors.

The source adapter remains compatible when the touch return value is corrected
from zero to four.
