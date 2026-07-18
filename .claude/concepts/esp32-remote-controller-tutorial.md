# ESP32 Remote Controller Tutorial on MicroWorld

## Problem

The current remote-controller guide is a roadmap rather than a tutorial, and the
combined redesign still teaches how to implement MicroWorld inside the ESP32
course. The owner needs a comprehensive, reproducible ESP32 learning path with
theory, reasoning, exact code, experiments, and failure evidence while building
the production controller inspired by `RadioRemoteController`.

## Proposed Approach

Replace the monolithic guide with 36 cumulative, focused tutorials grouped into
nine modules. Most tutorials target one hour of instruction. Hardware review,
range, electrical, security, and soak evidence gates may take repeated sessions
and are never rushed to fit the label.

The course treats a released MicroWorld package as a read-only prerequisite:

- tutorials may configure and use `FApplication`, `TWorld`, `TActor`,
  `FActorComponent`, `FNetwork`, lifecycle, and tick settings;
- tutorials implement the portable `remote_controller` application and the
  ESP32 platform shell;
- normal tutorial steps never modify `lib/microworld`;
- a required MicroWorld API change stops the tutorial, creates a separate
  framework change/version, and resumes only after that release passes.

Every tutorial contains, in order:

1. Result
2. Starting point
3. Theory
4. Reasoning and alternatives
5. Plan and prediction
6. Implementation
7. Build and verification
8. Failure exercise
9. Explain and record
10. Done when

All MicroWorld and tutorial C++ follows the released UE5-style naming contract:
`F` for non-UObject classes/structs, `T` for templates, `E` for enums, `b` for
booleans, unprefixed unit-explicit scalar aliases, and PascalCase public names.
The tutorial explains why MicroWorld does not use `A` or `U` prefixes without
UObject semantics. Every introduced class has a one-to-three-sentence Doxygen
comment describing purpose, ownership, or a critical invariant.

Each created source or documentation directory receives a concise, inheriting
`AGENTS.md`. The tutorial teaches the folder boundary at the moment it creates
the folder, and verification checks that no planned directory is missing its
local guide.

The curriculum progresses through:

1. ESP32-S3/ESP-IDF/PlatformIO build, boot, memory, partitions, and diagnostics;
2. MicroWorld consumption, its naming/performance contract, and the portable
   controller/valve application
   boundary;
3. GPIO, button observation, debounce, monotonic time, tick rates, FreeRTOS, and
   peripheral ownership;
4. fail-closed valve policy and required-output execution using an LED;
5. threat model and production security profile first, then explicit
   serialization, CRC, authentication, validation, and hostile-stream tests;
6. sessions, sequences, ACK/retry, heartbeat/status, link health, and a
   deterministic two-application simulator;
7. UART and exact E32 hardware/legal integration without premature RF;
8. production authentication/key provisioning, first authenticated E32
   exchange, and controller/valve composition;
9. radio fault/range, security recovery, reviewed valve driver, soak, and
   release evidence.

Embedded optimization is taught as measurement and trade-off, not a bag of
tricks. The learner records flash/static RAM, stack high-water marks, update
latency/cycles, buffer high-water marks, allocation count, radio airtime, and
power-relevant blocking. The course then applies bounded storage, single
peripheral ownership, batching, early exits, suitable compiler optimization/LTO,
and reduced work frequency only when evidence preserves clarity and all safety
deadlines. Bit packing, template expansion, virtual-dispatch removal, and
lower-width time arithmetic remain rejected until measurements justify their
complexity.
Dedicated controller/valve profile environments replay fixed host-oracle traces
with RF disabled and LED/fake valve output. Their uploads remain explicit
hardware gates; build size is not presented as observed timing, heap, or stack
behavior.

The product behavior comes from the useful parts of
`C:\Users\Public\Arduino\RadioRemoteController`: momentary hold-to-open and
release-to-close intent, explicit idempotent state messages, edge plus heartbeat
repair, connection health, reported output status, maximum-open protection,
link-loss closure, error lockout, and bounded radio recovery. The tutorial
deliberately hardens its unsafe or ambiguous parts: OFF is the first valve-side
hardware action, status does not claim physical valve movement without sensing,
old OPEN heartbeat cannot clear a trip, protocol input is authenticated and
replay-protected, and real hardware values are never copied without review.

## Open Questions

Exact board/carrier schematic, GPIO assignments, E32 SKU/settings, regional
rules, valve/driver circuit, timing constants, wire format, authenticator,
provisioning, and deployment hardware remain explicit tutorial evidence gates.
They are not guessed by the curriculum plan.

## Decisions Log

- 2026-07-18: Use comprehensive one-hour teaching units with theory, reasoning,
  code, verification, failure exercises, and objective completion gates.
- 2026-07-18: Keep high-level application behavior portable and ESP32 access at
  the imperative shell.
- 2026-07-18: Build the course around MicroWorld but do not implement MicroWorld
  inside it.
- 2026-07-18: Preserve the reference controller's useful behavior while
  replacing its Arduino/nRF24 assumptions and hardening safety/security.
- 2026-07-18: Write a separate tutorial plan after the standalone MicroWorld
  plan so the dependency and handoff are explicit.
- 2026-07-18: Critical valve safety evaluation is an unconditional application
  phase, not a configurable Actor/Component tick.
- 2026-07-18: Select all wire-affecting authentication/nonce/tag/replay
  parameters before freezing the protocol envelope; implement/provision the
  selected production profile before RF.
- 2026-07-18: Pin the tutorial to exact MicroWorld version `0.1.0` and the full
  source commit that passed native and ESP32 downstream probes.
- 2026-07-18: Use MicroWorld's adapted UE5 naming in every lesson and explain the
  `F`/`T`/`E`/`b` contract before product code.
- 2026-07-18: Add a measurement-led embedded optimization thread, concise class
  documentation, and a scoped `AGENTS.md` for every created folder.
