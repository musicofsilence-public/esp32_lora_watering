# ESP32 LoRa Remote Valve Controller — Contributor Guide

## Project purpose

Build a reliable two-device remote valve controller while teaching the owner
ESP32 development from first principles. The repository must progress through
small, observable experiments into production-quality firmware without hiding
the reasoning, measurements, or safety decisions.

The product is behaviorally inspired by the read-only project at
`C:\Users\Public\Arduino\RadioRemoteController`:

- A **controller unit** reads a momentary wall button. Holding the button
  expresses OPEN intent; releasing it expresses CLOSED intent.
- A **valve unit** receives that intent over E32 LoRa, applies a fail-closed
  safety policy, drives a reviewed external circuit for a 24 V solenoid valve,
  and reports its applied electrical output and safety state.

The reference is not a source to port literally. It uses Arduino Uno/Nano,
nRF24L01-specific behavior, conflicting pin/driver documentation, and
unvalidated hardware. This repository uses ESP32-S3, native ESP-IDF, modern
C++, PlatformIO, and an explicit E32 protocol.

The detailed learning sequence is in
`docs/esp32-lora-remote-controller-learning-guide.md`. Record predictions,
measurements, failure results, and accepted decisions in `LEARNING_LOG.md`.

## Product behavior

The target system must:

1. Start both units with CLOSED intent.
2. Keep the valve output electrically OFF through power-up, reset, boot, error,
   watchdog, and radio-recovery paths.
3. Require a stable released button before accepting a fresh OPEN press after
   controller boot.
4. Send explicit, idempotent OPEN/CLOSED state; never send a toggle command.
5. Send an immediate button-edge message and a periodic desired-state heartbeat
   so a lost press or release is repaired.
6. Report the valve unit's applied driver output and safety state. Do not call
   this physical valve position or water flow without a sensor.
7. Close on sustained link loss while open.
8. Enforce a hard continuous-open limit that repeated OPEN messages cannot
   extend.
9. Reject corrupted, malformed, stale, duplicated, replayed, out-of-range, or
   unauthenticated commands before they reach output control.
10. Prevent an old OPEN heartbeat from reopening after a fail-closed trip.
    Recovery requires valid CLOSED intent followed by a later fresh OPEN edge.
11. Recover transport failures in a bounded, observable way without letting
    radio reinitialization change valve state.

The heartbeat cadence, link-loss timeout, maximum-open duration, retry policy,
and recovery timing are unresolved safety/protocol decisions. Do not inherit
the reference's one-second, five-second, or five-minute values without
measurement and explicit approval.

## Current repository state

The application is at the first hardware learning exercise.
`src/main.cpp` blinks an external LED on GPIO10 using ESP-IDF GPIO and FreeRTOS
APIs. GPIO10 is an experiment-specific value, not an accepted product pin. Do
not flash or wire that exercise until GPIO10 is verified against the exact
carrier board.

Established technical decisions:

- Module: **ESP32-S3-WROOM-1-N16R8**
  - 16 MiB Quad-SPI flash
  - 8 MiB Octal-SPI PSRAM
- Framework: **ESP-IDF**, not Arduino
- Language for new application work: modern C++
- Build orchestration: **PlatformIO**
- Current board definition: `esp32-s3-devkitc-1`, with N16R8 overrides in
  `platformio.ini` and `sdkconfig.defaults`
- Flash mode/frequency: QIO at 80 MHz
- PSRAM mode/frequency: Octal at 80 MHz
- Serial monitor: 115200 baud
- Radio family: **E32 LoRa**; exact model and configuration unresolved
- License: MIT

The PlatformIO platform package is not currently pinned to an exact version.
Record the installed PlatformIO, Espressif platform, ESP-IDF, and toolchain
versions when building. Pinning/upgrading them is a later explicit
reproducibility decision, not part of an unrelated exercise.

The custom 16 MiB flash layout is:

| Partition | Size | Current purpose |
| --- | ---: | --- |
| `nvs` | 64 KiB | Reserved for small persistent settings |
| `otadata` | 8 KiB | OTA boot-slot selection metadata |
| `ota_0` | 4 MiB | First firmware image |
| `ota_1` | 4 MiB | Second firmware image |
| `storage` | 7.625 MiB | Reserved FATFS storage through wear levelling |
| `coredump` | 256 KiB | Crash diagnostics retained across reboot |

The layout is OTA-ready, but OTA download, signature validation, rollback, NVS,
FATFS, and core-dump reporting are not implemented.

## Source-of-truth rules

When sources disagree, use this order:

1. Safety invariants and explicitly accepted decisions in this file
2. Checked-in source, build configuration, and tests for current implemented
   behavior
3. The learning guide for sequence, experiments, and acceptance gates
4. `LEARNING_LOG.md` for actual observations and measurements
5. `RadioRemoteController` for behavioral inspiration
6. The reference project's wiring guide and schematic

The lower two items never override this repository's hardware or safety
decisions.

Known reference contradictions include:

- sketch/Markdown CE and valve pins differ from the schematic;
- the wiring guide uses a TIP142 Darlington while the schematic uses an
  IRLZ44N MOSFET;
- the reference reports an energized output but calls it actual valve state;
- link loss closes the reference output but an OPEN heartbeat may automatically
  reopen it after reconnection;
- the reference initializes its radio before forcing the valve output OFF;
- the reference README says real hardware validation is still pending.

Preserve the useful behavior while explicitly hardening these weaknesses.

## Hardware decision gates

Do not invent or copy unresolved hardware facts. Before implementing or
suggesting wiring, confirm:

- exact development/carrier board and schematic;
- ESP32 module and board pin mapping, including strapping, USB/JTAG, serial,
  flash, and PSRAM constraints;
- exact E32 SKU, manufacturer datasheet/manual revision, band, supply, logic
  levels, peak current, UART settings, AUX/mode timing, antenna, and regional
  rules;
- exact valve type, voltage, current, default mechanical state, duty/thermal
  limits, and plumbing behavior;
- driver topology, transistor conduction at the actual 3.3 V control level,
  gate/base network, flyback protection, fuse/current limiting, power supply,
  decoupling, grounding/isolation, connectors, and enclosure.

An ESP32-S3 GPIO is a logic signal. It must never source valve current or see the
24 V valve supply.

Never transmit from an E32 without the correct antenna attached. Do not change
radio configuration or transmit power until the exact module and applicable
regional constraints are documented.

## Architecture

Keep hardware access at the edges and domain rules independent of ESP-IDF where
practical. Introduce a boundary only when the active learning stage needs it.
Do not scaffold the final architecture up front.

Target logical boundaries:

- **input** — GPIO sampling plus pure button-debounce/arming behavior; emits
  typed press/release events
- **protocol** — explicit byte encoding/decoding, framing, validation, CRC,
  session/sequence logic, ACK matching, duplicate/stale handling, and counters;
  no GPIO or UART calls
- **radio** — exclusive ownership of E32 UART and mode/AUX signals; bounded
  transport and recovery; no valve decisions
- **valve_policy** — pure safety state machine for desired state, applied state,
  maximum-open deadline, link loss, lockouts, and deliberate recovery
- **valve_driver** — minimal reviewed adapter that applies safe OFF or requested
  ON to one GPIO; contains no radio policy
- **diagnostics** — reset reason, firmware/protocol version, link freshness,
  safety state/trips, and bounded health counters
- **controller app** — composition root for input, desired intent, heartbeat,
  status feedback, link indication, and radio
- **valve app** — composition root that forces safe OFF first, then connects
  validated messages to the safety policy and driver

The controller and valve applications are separate logical composition roots.
Whether PlatformIO builds them as separate environments, separate projects, or
another compile-time layout is unresolved until the second role is introduced.

Prefer one owner for each physical peripheral. Start with one non-blocking event
loop when it meets measured timing. Add FreeRTOS tasks/queues only for a concrete
ownership or blocking requirement; having two CPU cores is not sufficient
reason. Use a mutex only when single ownership is genuinely impossible.

Use modern C++ with clear ownership and deterministic lifetimes. Prefer value
types, `enum class`, fixed-width integers, bounded buffers, const-correctness,
explicit errors, and injected clocks for testable timing. Avoid exceptions,
unbounded blocking, dynamic allocation in steady-state control paths, global
mutable state, boolean state soup, and hidden hardware side effects.

## Core concepts

- Composition roots own concrete objects and connect narrow policy-free
  boundaries; dependencies point from application and hardware adapters toward
  pure domain/runtime code.
- Explicit state and typed results replace toggles, implicit ownership,
  exception-driven control flow, and ambiguous boolean state combinations.
- Caller-supplied monotonic time keeps scheduling, safety deadlines, and tests
  deterministic without hidden clock reads.
- Fixed-capacity storage and bounded work make MCU memory and timing behavior
  reviewable before deployment.
- MicroWorld is an independent lifecycle/tick package under `lib/microworld`;
  the tutorial may consume a pinned release later but must not redesign it
  during a lesson.

## Code documentation and formatting

- Format maintained C/C++ files with the tracked `clang-format` policy. Because
  the filename has no leading dot, invoke it explicitly with
  `clang-format --style=file:clang-format`.
- Document every function declaration and every persistent, shared,
  configuration, or state variable with why it exists, the ownership/lifecycle
  boundary it protects, or the invariant it makes observable.
- Use behavior-specific names for local variables. Add a local comment only
  when the reason, safety constraint, or edge case is not clear from code;
  never narrate obvious assignments or control flow.
- Every scoped `AGENTS.md` describes the architecture, concepts, dependency
  direction, and verification owned by its directory.

## Safety invariants

Code controlling water is safety-critical. Preserve these invariants through
every implementation and refactor:

- A hardware pull-down or equivalent keeps the driver OFF before firmware runs.
- The valve firmware's first hardware action forces the driver OFF. Do not log,
  initialize the radio, allocate services, or touch unrelated peripherals
  first.
- Global/static constructors must not energize or configure the valve output.
- Only the valve driver owns the physical output. All shutdown paths converge
  through its OFF operation.
- Boot/reset begins CLOSED and unarmed. A held button, stale OPEN heartbeat, or
  pre-reset intent cannot act as a fresh OPEN edge.
- Commands express OPEN or CLOSED intent. A remote toggle is forbidden.
- The maximum-open timer arms only on a real OFF-to-ON transition. Repeated OPEN
  commands and heartbeats never refresh it.
- Low-level radio recovery cannot bypass valve policy or change applied state.
- Sustained link loss while ON transitions to an OFF lockout.
- Malformed, unknown, out-of-range, stale, replayed, or unauthenticated input
  cannot open the valve or refresh command eligibility.
- After a fail-closed trip, remain OFF until a valid CLOSED intent is observed
  and a later fresh OPEN edge is accepted.
- A valid status message distinguishes desired state, applied electrical output,
  and safety state.
- Without a position/current/pressure/flow sensor, never claim the valve
  physically moved or water flowed.
- External valve circuitry provides a deterministic OFF bias, flyback
  protection, appropriate current limiting/fusing, and reviewed voltage/current
  margins. Firmware is not the only safety layer.
- Power loss, controller loss, E32 loss, protocol mismatch, watchdog reset,
  brownout, and error paths cannot leave the driver ON.
- Do not connect water or permit unattended operation until the electrical,
  protocol, safety, range, and soak gates have recorded results.

A conventional normally closed solenoid matches the initial fail-off model. A
latching valve needs a different two-pulse driver and state/recovery model; do
not substitute it without an explicit architecture and safety decision.

## Radio protocol requirements

E32 transport must explicitly handle corruption, loss, duplication,
delay/reordering, sustained silence, and unauthorized commands.

| Failure | Detection | Response |
| --- | --- | --- |
| Corruption | Framing/range checks and CRC | Reject |
| Loss | Matching ACK deadline | Bounded retry with calculated backoff/jitter |
| Duplication | Peer/session/sequence | Do not re-execute; ACK if appropriate |
| Delay/reordering | Session and freshness/sequence window | Reject stale intent |
| Silence | Link-freshness deadline | Fail closed if ON; bounded radio recovery |
| Unauthorized command | Authentication and replay policy | Reject and count |

Protocol code must:

- use explicit serialization; never transmit an ABI-dependent C++ object or
  packed struct as the accidental wire format;
- define framing, byte order, field widths, length bounds, identities, protocol
  version, session/boot epoch, sequence behavior, CRC coverage, and status
  semantics;
- validate enough framing to bound reads, then validate
  magic/version/identity/type/length/CRC/authentication/ranges/freshness before
  acting;
- separate immediate intent edges from desired-state heartbeats;
- match ACK/status to peer, session, and sequence;
- ensure repeated OPEN never extends the continuous-open limit;
- ensure an old OPEN heartbeat cannot clear a lockout;
- use bounded retry/backoff with jitter based on measured UART/radio latency and
  airtime;
- expose success, retry, duplicate, stale, CRC, validation, authentication,
  reinit, safety-trip, and final-failure counters;
- treat a wire-format change as a compatibility/version decision with updated
  golden-vector and behavior tests;
- distinguish CRC from authentication: CRC detects accidental corruption, not a
  valid-looking command from another transmitter;
- define a threat model, pairing, key provisioning/storage/replacement, message
  authentication, and replay protection before real-valve deployment.

The exact wire format and cryptographic primitive are unresolved. Do not select
them merely to make a documentation example concrete.

## Persistence, security, and observability

- Do not add persistence until a current requirement needs it.
- When NVS is introduced, initialize it with the ESP-IDF recovery path only for
  exhausted/incompatible NVS pages. Do not erase settings for unrelated errors.
- Store a schema version and validate every persisted value. Missing or invalid
  safety settings fall back to safe behavior.
- Define credential provisioning before storing radio authentication keys.
  Never commit secrets, production keys, credentials, or private material.
- Secure boot, flash encryption, NVS encryption, and eFuse changes require an
  explicit threat model, provisioning/recovery plan, and user approval. Some
  actions are irreversible.
- OTA needs signed-image policy, transport, validation, rollback, and recovery;
  the existing partitions alone do not implement OTA.
- Mount FATFS only after defining retention, full-filesystem behavior, and
  bounded write frequency.
- Diagnostics should eventually include role, firmware/protocol version, reset
  reason, uptime, peer/session identity, applied output, safety state, link age,
  safety trips, and protocol/radio counters.
- Logs must be bounded and useful without hiding time-critical behavior.

## Development and teaching workflow

Follow the active stage in the learning guide:

1. Explain the next concept and why the experiment exists.
2. Ask the owner to predict the result.
3. Resolve any hardware, safety, protocol, persistence, security, or
   architecture decision that affects the step.
4. Read the current code and implement only the agreed experiment.
5. Build and run the smallest relevant automated checks.
6. Provide the exact bench procedure and expected observation for hardware
   work.
7. Compare prediction with observation and explain the difference.
8. Record measured results in `LEARNING_LOG.md`.
9. State what was verified, what remains simulated, and the next gate.

Prefer one focused question at a time. Do not silently choose hardware or safety
details. Do not jump ahead by creating all components, a complete radio stack,
or real-valve code during an earlier lesson.

Learning code must be readable as teaching material:

- explain unfamiliar C++ syntax, ESP-IDF/FreeRTOS calls, execution flow, wiring
  assumptions, and expected hardware behavior;
- explain why a constraint or workaround exists, not what an obvious statement
  does;
- store intermediate values and `esp_err_t` results on descriptive lines before
  checking them;
- avoid dense nested calls, clever templates, premature abstractions, and
  unrelated refactors;
- keep pure policy tests concise and behavior-focused;
- remove obsolete teaching scaffolding when the learner no longer needs it and
  the cleanup is an agreed step.

## Build and verification

Primary current build:

```sh
pio run
```

Before considering a change complete:

- build every affected firmware environment;
- run host tests for pure protocol, timing, debounce, and valve-policy logic
  when present;
- run the narrowest relevant checks first, then broader checks when risk
  warrants;
- treat warnings as defects unless a documented toolchain issue requires a
  narrow suppression;
- search usages/callers/configuration/tests of changed code and assess behavior
  impacts;
- inspect touched ranges for clarity, duplication, unnecessary abstraction,
  mixed command/query behavior, and excessive coupling;
- provide concrete bench steps and expected observations for hardware changes;
- include a safe failure test for any output/valve/power change;
- distinguish build success, simulated behavior, bench measurements, and
  installed-system verification;
- never claim a build, test, measurement, range, security property, or hardware
  behavior that was not actually verified.

Do not upload firmware, erase flash, change radio configuration, transmit,
enable irreversible security/eFuse settings, energize the valve, connect water,
or push Git changes unless the user explicitly authorizes that action.

Documentation-only changes do not require a firmware build when no build input
changed. Verify their links, current-state claims, terminology, safety
traceability, and Markdown structure instead.

## Documentation synchronization

- `AGENTS.md` owns product invariants, accepted decisions, and contributor
  behavior.
- The learning guide owns stage order, experiments, evidence, and gates.
- `LEARNING_LOG.md` owns observations and measurements; do not copy assumptions
  there as results.
- `README.md` stays a concise orientation and links to the detailed sources.
- Checked-in code/config/tests remain authoritative for what is implemented.
- When an unresolved hardware or protocol decision becomes accepted, update
  this file, the relevant guide section, tests, and decision/measurement log in
  the same change.
- When behavior deliberately differs from `RadioRemoteController`, document the
  difference and safety rationale rather than editing the external reference.

## Unresolved decisions

- Exact development/carrier board and schematic
- Final GPIO assignment
- E32 model, band, radio settings, UART mode, supply, antenna, and regional rules
- Final application/source/PlatformIO layout for controller and valve roles
- Button wiring/protection for its real cable and installation
- Valve type and reviewed driver/power circuit
- Heartbeat, link-loss, maximum-open, retry, and radio-reinit timing
- Wire format, framing, session/sequence rules, and compatibility policy
- Threat model, authentication, pairing, and key provisioning/storage
- Indicator hardware and how it represents stale/unknown state
- Persistent settings and schema
- Logging/retention policy
- OTA/security/rollback policy
- Enclosure, environmental, wiring, and deployment requirements

Resolve these incrementally at the learning stage that needs them. Do not guess.
