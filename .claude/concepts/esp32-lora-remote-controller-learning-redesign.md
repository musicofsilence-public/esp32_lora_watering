# ESP32 LoRa Remote Controller Learning Redesign

## Problem

The repository's current guide teaches an autonomous garden-watering system:
soil sensing, battery operation, deep sleep, MQTT, and phone commands. That is a
different product from `C:\Users\Public\Arduino\RadioRemoteController`, whose
actual purpose is a two-device, always-available remote control for a 24 V water
valve: a wall-side momentary button expresses the desired valve state, and a
valve-side unit applies it with feedback and fail-closed watchdogs.

The current guide also targets an older ESP32-WROOM-32 with Arduino APIs, while
this repository is already configured for ESP32-S3-WROOM-1-N16R8, native
ESP-IDF, and PlatformIO. The reference project cannot be copied literally
either: it uses Arduino Uno/Nano and nRF24L01 hardware, its source, wiring guide,
and schematic disagree on pins and driver components, and its README says it
has not yet been validated on hardware.

## Proposed Approach

Redefine this repository as a comprehensive, experiment-led path from the
current ESP32-S3 LED exercise to a production-quality **ESP32 LoRa remote valve
controller**.

Use the reference project's behavior as the product target:

- A **controller unit** reads a debounced momentary wall button.
- Holding the button requests valve OPEN; releasing it requests valve CLOSED.
- Commands are explicit, idempotent set-state operations; there is no toggle.
- Periodic heartbeats repeat the current desired state so a dropped press or
  release edge is repaired after the link recovers.
- The valve unit reports its actual applied state, and both units expose link
  health.
- The valve defaults OFF, has a hard continuous-open limit, closes on sustained
  link loss, rejects malformed or stale commands, enters defined safety
  lockouts, and recovers only through an intentional state transition.
- Sustained radio failure triggers bounded recovery without ever bypassing the
  valve safety policy.

Keep the repository's accepted platform choices: ESP32-S3-WROOM-1-N16R8,
ESP-IDF, C++, PlatformIO, the current flash/PSRAM configuration, and E32 LoRa as
the proposed transport. Translate the nRF24 behavior into an explicit E32
protocol instead of imitating nRF24-specific hardware ACK payloads. The
protocol will eventually define serialization, versioning, CRC, sequence
numbers, acknowledgements, bounded retry/backoff, duplicate and stale-message
handling, actual-state feedback, and a deployment threat model with command
authentication.

Replace the current 74-day garden-system guide with a staged learning guide.
Each stage will use short labs with the same loop:

1. predict the behavior;
2. build or change one observable thing;
3. run the smallest check;
4. explain any difference;
5. record measured evidence in `LEARNING_LOG.md`.

The stages will progress through:

1. product contract, toolchain, build, flash, monitor, and debugging;
2. ESP32-S3 boot, memory, GPIO, and electrical limits;
3. modern C++ and host-testable logic;
4. LED output, button input, pull-ups, interrupts, and debouncing;
5. time, rollover-safe deadlines, non-blocking state machines, FreeRTOS tasks,
   queues, watchdogs, and ownership;
6. a simulated valve using only an LED and a pure safety state machine;
7. UART and E32 bring-up after the exact module and legal radio settings are
   confirmed;
8. a versioned, tested wire protocol and reliable delivery over an unreliable
   link;
9. controller-unit composition, desired-state heartbeats, feedback, and LEDs;
10. valve-unit composition, boot-safe output, link-loss behavior, maximum-open
    lockout, malformed-message lockout, and recovery;
11. reviewed valve-driver wiring and low-energy bench tests before connecting
    water or full valve power;
12. fault injection, range testing, soak testing, reset testing, diagnostics,
    release criteria, and deployment.

Soil sensing, autonomous watering, battery/deep-sleep operation, Wi-Fi, MQTT,
and phone dashboards will be removed from the core roadmap. They may be listed
only as optional future expansions after the reference controller behavior is
complete and verified.

Rewrite `AGENTS.md` around this target. It will define:

- product vocabulary and behavior;
- the current repository state (`src/main.cpp` is an external-LED learning
  exercise, not an empty C file);
- source-of-truth rules that treat `RadioRemoteController` as read-only
  behavioral reference, never trusted wiring;
- safety invariants and hardware decision gates;
- progressive component boundaries for input, protocol, radio, valve safety,
  diagnostics, and the two composition roots;
- the teaching workflow, verification expectations, and document-sync rules.

Rename the guide to
`docs/esp32-lora-remote-controller-learning-guide.md`, update `README.md` so the
repository does not advertise the old product, and remove the obsolete guide.
This redesign changes documentation only; it does not change firmware, build
configuration, wiring, flash contents, or external hardware.

## Open Questions

- The exact ESP32 development/carrier board, E32 model and band, regional radio
  settings, GPIO assignment, power topology, valve, and driver circuit remain
  unresolved. The guide will stop at explicit decision gates rather than guess.
- The reference uses a five-minute maximum continuous-open interval. The
  rewritten documents will treat the exact limit as a safety decision to
  confirm before real-valve work, not silently inherit either five or ten
  minutes.
- Whether both final units are mains-powered is not yet documented. The core
  controller will be designed as continuously available like the reference;
  battery and deep sleep will remain out of scope until power requirements are
  explicitly changed.

## Decisions Log

- 2026-07-18: Treat `RadioRemoteController` as the behavioral target, not a
  source to copy — it uses different hardware and contains contradictory,
  unvalidated wiring information.
- 2026-07-18: Preserve ESP32-S3, native ESP-IDF, C++, PlatformIO, and the
  checked-in N16R8 memory configuration — these are established repository
  decisions and appropriate learning targets.
- 2026-07-18: Recommend retaining E32 LoRa while reproducing the reference
  controller semantics through an explicit software protocol — nRF24 hardware
  ACK behavior is not portable to E32.
- 2026-07-18: Remove autonomous soil watering, battery/deep-sleep operation,
  MQTT, and phone control from the core product — they are responsible for the
  present mismatch.
- 2026-07-18: Keep this redesign documentation-only — implementation begins
  later as small, agreed learning exercises.
- 2026-07-18: Concept approved by the owner — retain ESP32-S3, ESP-IDF,
  PlatformIO, and E32 LoRa; target momentary hold-to-open/release-to-close
  behavior; remove autonomous watering, deep sleep, and MQTT from the core
  roadmap.
