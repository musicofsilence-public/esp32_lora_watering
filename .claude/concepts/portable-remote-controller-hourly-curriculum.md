# Portable Remote Controller Hourly Curriculum

> **Superseded on 2026-07-18.** Framework implementation and ESP32 teaching are
> now planned separately in
> [`microworld-framework.md`](microworld-framework.md) and
> [`esp32-remote-controller-tutorial.md`](esp32-remote-controller-tutorial.md).
> This file remains as decision history.

## Problem

The current document is a roadmap: it says what to learn and what to verify, but
it does not teach the material. It lacks a repeatable one-hour lesson structure,
incremental compilable code, exact commands, and enough theory to explain why
each design choice exists. Its architecture section also describes separation
in prose without making portability the organizing rule of the code.

## Proposed Approach

Replace the current learning document with a structured 36-tutorial curriculum.
Organize it as an index, one architecture chapter, and nine modules containing
four one-hour tutorials each. Every tutorial will start from the previous
tutorial's working state and add one tested vertical increment toward the final
remote valve controller.

Every one-hour tutorial will use the same structure:

1. **Result** — the concrete working behavior produced this hour.
2. **Starting point** — files and tests that must already exist.
3. **Theory** — the ESP32/C++/radio concept in plain language.
4. **Reasoning** — why this concept is introduced now and why the selected
   design is preferable to realistic alternatives.
5. **Plan and prediction** — what the learner expects before running code.
6. **Implementation** — exact file paths, incremental C++ code, and commands.
7. **Verification** — automated tests or a bounded bench observation.
8. **Failure exercise** — intentionally break one relevant assumption.
9. **Explain and log** — questions answered in `LEARNING_LOG.md`.
10. **Done when** — an objective acceptance check and the next tutorial.

One hour means one focused instructional unit, not a promise that physical
evidence takes one hour. Hardware tutorials may pause and resume for repeated
measurement, fault, range, soak, or review sessions.

Make portability structural through a small **UE5-inspired functional core /
imperative shell** architecture. Borrow UE5's vocabulary and ownership model,
not its reflection, garbage collection, rendering, transforms, or dynamic object
system:

- `ControllerApplication` and `ValveApplication` are concrete portable
  lifecycle/composition coordinators. Each owns its concrete `World` and a
  `Network`, sequences `BeginPlay`, `Tick`, and shutdown, and returns
  role-specific outputs for the platform to execute.
- `ControllerWorld` and `ValveWorld` are portable logical worlds, not scenes or
  physics engines. They own a small, fixed actor set and route typed events.
- `ControllerActor` and `ValveActor` are high-level entities. Each explicitly
  owns only the components its role needs.
- Components are focused pieces of behavior such as
  `ButtonIntentComponent`, `ValveSafetyComponent`, `LinkHealthComponent`, and
  `StatusIndicatorComponent`. Components contain no GPIO/UART code.
- MicroWorld `Network` infrastructure owns authenticated framing, validation,
  sessions/sequences, ACK/retry, and link statistics. Desired-state heartbeat
  and valve status remain remote-controller application policy. Network
  consumes byte ranges and produces bounded transmit data; it never owns an
  ESP32 UART.
- `lib/microworld/` contains reusable C++17 time, bounded-buffer, byte-view, and
  Network infrastructure. `lib/remote_controller/` contains the applications,
  worlds, actors, components, protocol messages, and controller policy. Neither
  includes ESP-IDF, FreeRTOS, Arduino, GPIO, UART, or E32 headers.
- Each application tick receives a role-specific plain input containing time,
  observations, received bytes, and prior adapter-execution feedback.
  Controller output may contain bounded transmit/indicator/diagnostic work.
  Valve output always has a dedicated required state plus bounded best-effort
  work.
- The ESP32 valve shell forces OFF before application startup, executes the
  required valve output before best-effort effects, and returns adapter-call
  success/failure to the next tick. This does not claim electrical or physical
  movement without measurement. Failed ON/OFF calls enter a tested
  platform-specific emergency-safe/reset path. A full queue can never suppress
  OFF.
- `test/` runs the same core on the desktop with fake time and an in-memory
  lossy link. The end-to-end simulator runs two portable `Application`
  instances—controller world and valve world—without ESP32 code.
- `src/platform/esp32/` implements the imperative shell: ESP-IDF GPIO, monotonic
  clock, UART/E32 transport, logging, bounded scheduling, input collection, and
  effect execution.
- Explicit controller/valve composition sources construct the correct
  applications and bind platform inputs/outputs. Separate PlatformIO
  `controller` and `valve` environments prove each role.
- A future MCU port supplies a new shell/adapters while reusing the core and its
  tests unchanged.

Keep the object model deliberately explicit:

- Actors are created at startup and live for the application lifetime.
- A concrete actor owns concrete component members; adding a component is an
  intentional code change visible in that actor.
- `Application`, `World`, `Actor`, and Actor Component begin as concrete roles
  with direct calls. `BeginPlay`, `Tick`, and `EndPlay` are a lifecycle naming
  convention, not a required virtual hierarchy.
- There is no dynamic actor spawning, general component registry, reflection,
  string lookup, global event bus, service locator, transform tree, or heap-based
  UObject clone.
- Typed events and explicit outputs replace hidden cross-object calls. Only
  best-effort work uses bounded queues; required valve output is separate.
- If a general engine feature is not required by the remote controller or a
  demonstrated extension, it is not built.

The portability contract is **any C++17-capable microcontroller with sufficient
resources and adapters**, not literally every MCU. Portable code will avoid
exceptions, RTTI-dependent design, operating-system types, heap allocation in
steady state, unbounded containers, platform byte order, and hardware-specific
error types.

Use nine cumulative modules:

1. **Build and mental model** — toolchain, boot, `app_main`, first GPIO, product
   failure stories.
2. **Portable application/world foundation** — library boundary, `Application`,
   `World`, actors, explicit components, typed events/effects, native tests, and
   a fake platform tick using actual controller/valve behavior, not throwaway
   architectural rehearsal.
3. **Time and controller input** — rollover-safe deadlines, raw button
   observation, pure debounce, boot arming, and a controller actor/component.
4. **Valve safety policy** — explicit states, maximum-open rule, link/error
   lockouts, deliberate recovery, a valve actor/component, and ESP32 LED adapter.
5. **Authenticated wire protocol** — threat model/authenticator boundary first,
   then explicit serialization, CRC, authentication coverage, framing,
   validation, golden vectors, hostile-stream tests, and portable `Network`.
6. **Reliable application link** — session/sequence, ACK, retry/backoff,
   heartbeat/status, world/network event routing, and an end-to-end lossy host
   simulation with two application instances.
7. **ESP32 runtime adapters** — FreeRTOS timing/ownership, bounded queues where
   justified, UART loopback, reset/watchdog diagnostics.
8. **E32 and firmware composition** — hardware/legal gate, implemented and
   vector-verified production authentication/replay rejection, recoverable key
   provisioning, first authenticated radio exchange, controller firmware, and
   valve firmware with simulated output.
9. **Production gates** — repeated radio evidence, authentication/key-recovery
   hardening, reviewed valve driver, real-load soak, and release evidence.

Each module will contain real code that evolves consistently. Early tutorials
will create and test the portable application framework before ESP32 adapters
are allowed to depend on it. Hardware examples will use clearly marked,
datasheet-approved configuration values established during the relevant gate;
they will not smuggle in guessed GPIO or E32 settings.

Rewrite `AGENTS.md` to enforce the dependency rule:

```text
ESP32 shell: GPIO / UART / clock / logging
                │ plain inputs and effects
                ▼
portable Application
       ├── World
       │    ├── Actor
       │    └── explicit Components
       └── Network
```

Build dependencies point toward the portable core. Runtime data crosses the
boundary as plain inputs, received bytes, and returned effects. The core never
includes or calls outward into ESP-IDF. A new microcontroller port changes the
imperative shell and composition binding only.

Replace the single large guide with:

```text
docs/learning-guide/
├── README.md
├── architecture.md
├── verification.md
├── module-01-build-and-mental-model.md
├── module-02-portable-core-foundation.md
├── module-03-time-and-controller-input.md
├── module-04-valve-safety-policy.md
├── module-05-wire-protocol.md
├── module-06-reliable-application-link.md
├── module-07-esp32-runtime-adapters.md
├── module-08-e32-and-firmware-composition.md
└── module-09-production-gates.md
```

Update README and the learning-log template to point to the modular curriculum.
This documentation redesign will not yet implement the tutorial code in the
repository; following the tutorials will create it incrementally.

During authoring, checkpoints 1–3 must reproduce the starting ESP32 build.
Tutorial 4 creates `native`, `controller`, and `valve`; checkpoints 4–36 must
run relevant native tests and build both ESP32 roles in an isolated cumulative
workspace. `verification.md` records code/build status separately from physical
evidence.

## Open Questions

None for the documentation architecture. Exact GPIO, E32, radio, protocol,
security, and valve decisions remain tutorial gates rather than curriculum
design questions.

## Decisions Log

- 2026-07-18: One-hour step-by-step tutorials are required — each must contain
  theory, reasoning, code, verification, and a concrete move toward the product.
- 2026-07-18: Application logic must be reusable beyond ESP32 — microcontroller
  APIs belong in adapters/composition, not the application core.
- 2026-07-18: The current roadmap-style guide is rejected — it will be replaced,
  not lightly edited.
- 2026-07-18: Use UE5-inspired application/world/network/actor/component
  concepts for extension and separation — keep only the minimal ownership and
  lifecycle ideas that make this controller simpler.
- 2026-07-18: Name the portable engine **MicroWorld** and use the `microworld`
  namespace — the remote controller remains a separate application layer.
- 2026-07-18: Revised concept approved — proceed with 36 cumulative one-hour
  tutorials, familiar `BeginPlay`/`Tick`/`EndPlay` lifecycle, cumulative code,
  and hardware values gated until verified.
- 2026-07-18: Architecture refinement after adversarial review — MicroWorld
  roles begin as concrete classes with direct ownership rather than mandatory
  virtual bases; the valve output is a dedicated non-droppable request with
  execution feedback.
- 2026-07-18: Verification/security refinement — all tutorial checkpoints build
  the environments available at that point (starting ESP32 for 1–3,
  native/controller/valve for 4–36); authentication and replay policy precede
  the wire envelope, and implemented/vector-verified production authentication,
  replay rejection, and key provisioning precede RF.
