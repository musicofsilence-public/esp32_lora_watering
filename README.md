# ESP32 LoRa Remote Valve Controller

A learning project that grows from small ESP32-S3 experiments into a
production-quality, two-device remote control for a 24 V water valve.

## Product target

- The **controller unit** reads a momentary wall button: hold requests OPEN,
  release requests CLOSED.
- The **valve unit** receives intent through E32 LoRa, applies a fail-closed
  safety policy, controls a reviewed external driver, and reports its applied
  electrical output and safety state.
- Explicit state, heartbeats, acknowledgements, retries, lockouts, and
  diagnostics make failures observable and bounded.

This is inspired by `C:\Users\Public\Arduino\RadioRemoteController`, but it is an
ESP32-S3/ESP-IDF redesign rather than an Arduino/nRF24 port.

## Current status

The controller and valve firmware are not implemented yet. `src/main.cpp` is the
first ESP-IDF learning exercise: blinking an external LED. No button, E32 link,
valve driver, or real valve behavior has been validated by this repository.

MicroWorld 0.2.0 is an external dependency developed in the sibling repository
at [`../MicroWorld`](https://github.com/musicofsilence-public/MicroWorld). This
firmware consumes it through `symlink://` `lib_deps` in `platformio.ini`, so
both repositories must be cloned side by side:
`projects/lora` and `projects/MicroWorld`. The engine is not yet linked into
`src/main.cpp`, and the ESP32 tutorial has not begun consuming it.

## Confirmed platform

- ESP32-S3-WROOM-1-N16R8
- 16 MiB Quad-SPI flash and 8 MiB Octal-SPI PSRAM
- ESP-IDF with modern C++
- PlatformIO using the current `esp32-s3-devkitc-1` definition and explicit
  N16R8 configuration
- E32 LoRa radio family; exact model and settings remain unresolved
- Serial monitor at 115200 baud

## Build

```sh
pio run
```

Building does not flash the board. Uploading, transmitting, changing radio
configuration, or energizing a valve requires a separate intentional hardware
step.

## Learn and contribute

- [Learning guide](docs/esp32-lora-remote-controller-learning-guide.md)
- [Learning and measurement log](LEARNING_LOG.md)
- [MicroWorld engine](https://github.com/musicofsilence-public/MicroWorld)
- [Contributor and safety rules](AGENTS.md)

## Safety

The ESP32 must never drive a 24 V valve directly. The final system requires a
reviewed driver, deterministic hardware OFF bias, flyback protection,
appropriate power protection, an attached radio antenna, and recorded bench
tests. Reported output state does not prove mechanical valve position or water
flow without a sensor.

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE).
