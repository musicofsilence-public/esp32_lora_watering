# ESP32 LoRa Watering — Contributor Guide

## Project purpose

Build a reliable remote watering system while teaching the owner ESP32
development step by step. The repository must grow from small, observable
experiments into production-quality firmware without hiding the reasoning behind
the design.

The finished system has two ESP32-S3 devices:

- A battery-powered **field node** measures soil moisture and battery voltage,
  makes safe local watering decisions, controls a valve, exchanges messages over
  an E32 LoRa radio, and spends most of its time in deep sleep.
- An always-on **base station** bridges LoRa telemetry and commands to MQTT over
  Wi-Fi. It lets a dashboard or phone observe the node and request watering.

The field node must remain useful when the base station, Wi-Fi, MQTT broker, or
internet is unavailable. Remote communication improves the system; it must not
be a single point of failure for basic watering.

The detailed learning roadmap is in
`docs/esp32-lora-watering-daily-guide.md`. Treat it as the source for product
goals, safety reasoning, experiments, and acceptance checks—not as directly
compilable code for this repository.

## Current state

The application is intentionally almost empty. `src/main.c` contains only an
empty `app_main()`. Build configuration for the real module is established and
has been verified with PlatformIO.

Accepted technical decisions:

- Confirmed module: **ESP32-S3-WROOM-1-N16R8**
  - 16 MiB Quad-SPI flash
  - 8 MiB Octal-SPI PSRAM
- Framework: **ESP-IDF**, not Arduino
- Build orchestration: **PlatformIO**
- PlatformIO board definition: `esp32-s3-devkitc-1`, with explicit N16R8
  overrides in `platformio.ini` and `sdkconfig.defaults`
- Flash mode/frequency: QIO at 80 MHz
- PSRAM mode/frequency: Octal at 80 MHz
- Serial monitor: 115200 baud
- License: MIT

The custom 16 MiB flash layout in `partitions.csv` is:

| Partition | Size | Purpose |
| --- | ---: | --- |
| `nvs` | 64 KiB | Small persistent key-value settings |
| `otadata` | 8 KiB | OTA boot-slot selection metadata |
| `ota_0` | 4 MiB | First firmware image |
| `ota_1` | 4 MiB | Second firmware image |
| `storage` | 7.625 MiB | FATFS files through wear levelling |
| `coredump` | 256 KiB | Crash diagnostics retained across reboot |

The layout makes the firmware OTA-ready, but OTA download and rollback behavior
have not been implemented. NVS and FATFS are reserved but are not yet initialized
or used by application code.

## Source-of-truth rules

When documents disagree, use this order:

1. Safety requirements and accepted decisions in this file
2. The checked-in build configuration and tests
3. The product intent and learning sequence in the daily guide
4. Code snippets and hardware examples in the daily guide

The daily guide was originally written for ESP32-WROOM-32 and Arduino. This
repository uses ESP32-S3-N16R8 and ESP-IDF. Therefore:

- Port concepts to native ESP-IDF APIs; do not add Arduino compatibility merely
  to copy a guide example.
- Do not copy the guide's `esp32dev`, pioarduino, `setup()`/`loop()`,
  `Preferences`, `Serial2`, Wi-Fi, ADC, GPIO, or deep-sleep calls unchanged.
- Do not copy its GPIO table. ESP32-S3 pin capabilities differ, and the N16R8
  module's Octal PSRAM reserves module pins that older ESP32 examples may use.
- Verify every GPIO against the exact development/carrier-board schematic and
  the ESP32-S3-WROOM-1 datasheet before implementing a driver or suggesting
  wiring.
- Recalculate radio airtime, retry timeout, duty-cycle limits, ADC scaling, and
  battery thresholds from the chosen hardware and regional radio rules.

## Architecture

Keep hardware access at the edges and domain logic independent of ESP-IDF where
practical. Pure protocol, validation, conversion, and watering-policy code
should be testable on the host without an ESP32.

Use modern C++ with clear ownership and deterministic lifetimes for new
application code. Prefer value types, RAII, const-correctness, fixed-width
integers, bounded buffers, and explicit error handling. Avoid exceptions,
dynamic allocation in steady-state control paths, global mutable state, and
unbounded blocking. Use C only where a direct ESP-IDF C boundary makes it
simpler. Convert the application entry point to `main.cpp` when C++ application
work begins.

Target component boundaries:

- **protocol** — versioned wire messages, serialization, CRC, sequence numbers,
  duplicate detection, ACK/retry policy, and link statistics; no hardware calls
- **radio** — E32 UART/GPIO ownership and transport; depends on protocol
- **settings** — validated configuration persisted in NVS, including schema
  version and migration/default behavior
- **soil** — powered sampling, filtering, calibration, and conversion to a
  bounded moisture percentage
- **power** — battery measurement and deep-sleep policy
- **watering** — valve control and pure safety/decision policy
- **storage** — FATFS mounting and bounded diagnostic/log retention
- **diagnostics** — reset reason, core-dump awareness, health counters, and
  telemetry
- **field-node app** — composition root and wake-cycle state machine
- **base-station app** — composition root, LoRa/MQTT coordination, and pending
  command management

Do not create all components up front. Introduce a boundary when a roadmap step
needs it, and keep its public API minimal.

### Field-node flow

The target wake cycle is:

1. Force the valve to its safe OFF state before other initialization.
2. Initialize required services and load validated settings.
3. Power and sample the soil sensor; measure battery voltage.
4. Decide locally whether watering is allowed and necessary.
5. Execute a bounded watering operation when required.
6. Send telemetry through the reliable LoRa protocol.
7. Keep a short receive window for a pending command from the base.
8. Put the E32 into sleep mode and enter ESP32 deep sleep.

State that must survive deep sleep, such as message sequence numbers and the last
executed command, belongs in RTC memory when appropriate. Configuration that
must survive power loss belongs in NVS. Logs or larger records belong in FATFS.

### Base-station flow

The base station runs continuously and has two independent responsibilities:

- A radio task exclusively owns the E32 interface, validates packets, sends
  acknowledgements, and delivers eligible pending commands while a node is
  awake.
- A network task owns Wi-Fi and MQTT, publishes telemetry, accepts commands, and
  handles reconnect and resubscription.

The tasks communicate through typed FreeRTOS queues. Telemetry may use a
documented drop-old/drop-new policy because stale readings lose value. Commands
must never disappear silently: enqueue failure, expiry, delivery, and rejection
must be observable.

Prefer one task as the sole owner of each physical peripheral. Add a mutex only
when single ownership is genuinely impossible.

## Safety invariants

Code that controls water is safety-critical for this project. Preserve these
invariants through every refactor:

- The valve output is forced OFF as the first hardware action after every boot,
  reset, or wake.
- The external valve circuit must provide a gate pull-down and flyback
  protection; firmware is not the only safety layer.
- Every watering operation has a hard maximum duration. The guide currently
  proposes 600 seconds; changing it requires an explicit decision and tests.
- Low or invalid battery readings prevent watering rather than permitting it.
- Invalid, uncalibrated, or implausible soil readings must not trigger
  uncontrolled watering.
- Commands express an idempotent action such as “water for N seconds.” Never
  implement a remote “toggle valve” command.
- The base rejects expired commands because it has wall-clock time. The sleeping
  node rejects duplicate sequence numbers because it may not know real time.
- Loss of LoRa, Wi-Fi, MQTT, or the base station cannot leave the valve open.
- Error paths and watchdog recovery must converge to valve OFF.
- Never transmit from an E32 module without the correct antenna attached.
- Never assume the 12 V valve supply is safe for an ESP32 pin. The power stages
  share ground only where the reviewed circuit requires it.

Prefer a latching valve for the eventual battery installation. A conventional
solenoid is acceptable for learning and bench testing if its current and thermal
limits are respected.

## Radio protocol requirements

The E32 link must explicitly handle all three expected failure modes:

| Failure | Detection | Response |
| --- | --- | --- |
| Corruption | CRC | Reject the packet |
| Loss | ACK timeout | Retry with a bounded attempt count |
| Duplication | Sequence number | Ignore an already executed command |

Protocol code must:

- Use an explicit, versioned wire format; never transmit an ABI-dependent C++
  object accidentally.
- Define byte order, field widths, maximum payload, framing, and CRC coverage.
- Validate magic/version/type/length/ranges before acting on a packet.
- Match acknowledgements to sender and sequence number.
- Use bounded retry backoff with random jitter to avoid repeated collisions.
- Keep success, retry, rejection, CRC-failure, and final-failure counters.
- Treat a wire-format change as a compatibility change requiring a protocol
  version decision and updated tests.

The 24-byte packed packet in the guide is a starting concept, not yet an accepted
wire contract. Review its serialization and framing before adopting it.

## Persistence and observability

- Initialize NVS using the standard ESP-IDF recovery path for exhausted or
  incompatible NVS pages. Do not erase settings for unrelated errors.
- Store a configuration schema version and validate every loaded value. Fall
  back to safe defaults when data is missing or invalid.
- Mount the `storage` partition using ESP-IDF FATFS plus wear levelling. Define
  retention and full-filesystem behavior before writing recurring logs.
- Never store secrets, credentials, or private keys in Git. Use a documented
  local configuration mechanism and provide non-secret examples.
- Log reset reason, wake cause, firmware version, free/internal memory, PSRAM
  status, and critical subsystem failures.
- Diagnostics must remain useful without serial access: important health and
  link statistics eventually belong in telemetry.
- Check every ESP-IDF return value. Expected failures should have an explicit
  recovery or safe-degradation policy.

## Development workflow

The owner is learning ESP32 development. Work in small, reviewable steps:

1. Explain the next decision in plain language and why it matters.
2. Ask before making a choice with meaningful hardware, safety, persistence,
   protocol, or architectural consequences.
3. Implement only the agreed step.
4. Build and run the smallest relevant automated checks.
5. State what was verified, what still requires hardware, and what comes next.

Do not silently fill in unresolved hardware details. Prefer one focused question
at a time. Preserve the guide's learning method: predict behavior first, observe
the result, and explain any difference.

Learning exercises must be extensively commented. Explain unfamiliar C++ syntax,
ESP-IDF and FreeRTOS calls, execution flow, wiring assumptions, and expected
hardware behavior so the source can be read as learning material. Keep the
executable logic simple and make each comment teach purpose or reasoning.
Avoid nested function calls in learning exercises. Store intermediate values and
return codes in descriptively named variables so each operation and its error
check can be read separately.

Comments must explain **why** a constraint, workaround, or non-obvious decision
exists. Do not add comments that merely restate the code. Use the established
`Why:` style in configuration files where it improves clarity.

Keep `LEARNING_LOG.md` when practical, recording observed values rather than
assumptions: ADC measurements, calibration endpoints, stack high-water marks,
radio airtime, retry counts, current consumption, and failure-test results.

## Build and verification

Primary build:

```sh
pio run
```

Before considering a change complete:

- Build every affected firmware environment.
- Run host tests for hardware-independent logic when present.
- Add tests for protocol serialization/validation and safety policy before
  relying on those parts on hardware.
- Treat warnings as defects unless a documented toolchain issue requires a
  narrowly scoped suppression.
- For hardware changes, provide a concrete bench test and expected observation.
- For valve/power work, include a safe failure test and do not claim verification
  without physical measurement.
- Do not upload firmware, erase flash, change radio configuration, energize the
  valve, or push Git changes unless the user explicitly authorizes that action.

## Unresolved decisions

Resolve these incrementally; do not guess:

- Exact development/carrier board and its schematic (the module is confirmed as
  ESP32-S3-WROOM-1-N16R8)
- Final GPIO assignment for E32, soil sensor, battery ADC, and valve driver
- E32 model, frequency band, transmit power, UART mode pins, and regional rules
- Soil-sensor electrical characteristics and measured wet/dry calibration
- Battery chemistry, voltage-divider ratio, low-voltage threshold, and power
  switching
- Conventional versus latching valve and the corresponding driver circuit
- MQTT broker, authentication/TLS, topic contract, and credential provisioning
- Exact protocol wire format
- Logging/retention policy for the FATFS partition
- OTA transport, signature/security policy, validation, and rollback behavior

When one of these becomes known, update this file and the relevant tests or
documentation in the same change.
