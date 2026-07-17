# ESP32 LoRa Watering

An ESP-IDF learning project that grows into a remote watering system using
ESP32-S3 boards and E32 LoRa radios.

## Target hardware

- ESP32-S3-WROOM-1-N16R8
  - 16 MB Quad-SPI flash
  - 8 MB Octal-SPI PSRAM
- E32 LoRa radio modules
- Capacitive soil-moisture sensor
- Logic-level MOSFET driver and 12 V solenoid valve

## Development environment

- [PlatformIO](https://platformio.org/)
- [ESP-IDF](https://github.com/espressif/esp-idf)
- PlatformIO board: `esp32-s3-devkitc-1`

The project configuration enables the N16R8 module's 16 MB flash and 8 MB
PSRAM. The persistent ESP-IDF settings are stored in `sdkconfig.defaults`.

## Build

```sh
pio run
```

Upload and open the serial monitor:

```sh
pio run --target upload
pio device monitor
```

## Learning guide

See the [daily ESP32 and LoRa watering guide](docs/esp32-lora-watering-daily-guide.md).

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE).
