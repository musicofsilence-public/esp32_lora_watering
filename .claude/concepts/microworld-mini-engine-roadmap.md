# MicroWorld Mini Engine

> **Status and authority.** This is the active post-0.1 architecture concept.
> Current implementation and gate state are owned by
> [MicroWorld progress](../../lib/microworld/PROGRESS.md); the executable
> sequence is the [mini-engine roadmap](../plans/microworld-mini-engine-roadmap.md).
> This concept records durable decisions, not live progress.

## Problem

MicroWorld 0.1.0 is a small, deterministic lifecycle and primary-tick kernel.
It deliberately excludes the managed object model, garbage collection, dynamic
spawning, smart-pointer family, reflection metadata, platform adapters, and
network stack now required by the product vision. Adding those features
directly to the current classes would either turn every application into a
desktop-style runtime or blur the package's existing bounded ownership and
platform-neutral guarantees.

MicroWorld therefore needs a new architectural target: it should feel familiar
to a UE5 C++ developer while remaining native to constrained microcontrollers.
"Familiar" must mean transferable concepts, vocabulary, lifecycle, and coding
patterns rather than UE5 source compatibility or a reduced copy of desktop
engine internals.

## Proposed Approach

Adopt a **layered, UE-shaped, MCU-native runtime**. Preserve the current v0.1
kernel as proven foundation, split future capabilities into separately linkable
modules, and make every dynamic resource bounded and observable.

### Options considered

| Option | Direction | Strengths | Costs | Recommendation |
| --- | --- | --- | --- | --- |
| Literal miniature UE5 clone | Reproduce UObject, reflection, GC, replication, task graph, and engine structure as closely as possible | Maximum superficial familiarity | Desktop assumptions, high RAM/flash cost, difficult ports, misleading compatibility, long time before a useful release | Reject |
| Layered UE-shaped runtime | Reuse UE mental models and selected names over fixed arenas, explicit metadata, bounded work, and platform adapters | Familiar, portable, measurable, and usable from tiny to connected builds | Requires carefully documented semantic differences from UE5 | **Select** |
| Minimal kernel plus unrelated libraries | Keep v0.1 unchanged and add separate utility/network libraries with no unified runtime | Lowest framework risk and smallest core | Does not deliver the coherent UE-like developer experience in the vision | Reject |

### Target module boundaries

- **Core** — retain released lifecycle, caller-supplied monotonic time, typed
  results, deterministic ticking, bounded registration, and minimal
  diagnostics.
- **Memory** — own injected memory resources, fixed arenas/storage utilities,
  bounded containers/spans/delegates, and allocator-aware `TUniquePtr`,
  `TSharedPtr`, and `TWeakPtr` for non-object ownership.
- **Object** — own `UObject` identity, stable handles, descriptors,
  `TObjectPtr`, `TWeakObjectPtr`, `TStrongObjectPtr`, the object store, roots,
  and bounded GC. Actor, Component, World, and spawning behavior stay in Engine.
- **Engine** — own `UWorld`, `AActor`, `UActorComponent`, subsystems, timers,
  managed lifecycle, and bounded spawn/destroy queues. Deterministic hardware
  and safety services remain consumer-owned objects outside GC.
- **Serialization** — provide explicit bounded byte archives with named widths,
  byte order, lengths, versions, and failures.
- **Net** — own `INetDriver`, `FNetManager`, sessions, validation, and bounded
  queues. It depends inward on Core, Memory, and Serialization, never Engine;
  Engine-Net scheduling belongs to a separate bridge.
- **Platform contracts and ports** — keep the engine modules free of ESP-IDF,
  STM32 HAL, Pico SDK, Arduino, or RTOS headers. Downstream port packages adapt
  clocks, memory diagnostics, entropy, critical sections, logging, byte
  streams/datagrams, and selected peripherals for host, ESP32, STM32, and
  RP2040 builds.
- **Developer experience** — provide project templates, host simulation,
  examples, API mapping from UE5 concepts, resource reports, compile-time
  configuration, and migration documentation from MicroWorld 0.1.

### Resource and GC model

The managed-object module uses a caller-supplied, fixed-capacity arena by
default. Collection is non-moving, explicit-root, metadata-guided, and
incremental with a caller-selected work budget. It never scans arbitrary stack
memory, runs in an ISR, silently grows the heap, or makes out-of-memory
unobservable. A full collection is an explicit operation; normal engine
updates perform bounded slices.

GC is an opt-in capability, not a requirement of the core-only profile.
Hardware drivers, interrupt-facing data, network buffers, watchdog paths, and
safety-critical state machines use deterministic values, fixed storage, or
unique ownership. `TObjectPtr` participates in GC; `TSharedPtr` uses reference
counting; non-owning raw pointers are restricted to documented,
pointer-stable scopes. The APIs must make these different lifetime models hard
to confuse.

### Product profiles

- **Core** — released lifecycle, tick, time, results, bounded registration, and
  minimal diagnostics.
- **Memory** — Core plus injected resources, utilities, bounded containers and
  delegates, and non-object smart ownership.
- **Object** — Core plus Memory, managed identity, references, storage, roots,
  and bounded GC.
- **Managed** — Core plus Memory, Object, and Engine.
- **Core+Net** — Core plus Memory, Serialization, and Net.
- **Managed+Net** — Managed plus Serialization and Net, with the optional
  Engine-Net bridge when Engine scheduling is required.

Profiles are bundles of separately linkable targets rather than one
preprocessor-heavy monolith. Link-time dead stripping remains useful, but each
module also has an explicit RAM, flash, stack, allocation, and worst-case work
budget.

### Compatibility and scope

MicroWorld 0.1 remains a valid deterministic release while the new modules are
built incrementally. Source compatibility is not promised before 1.0, but each
breaking release needs a migration guide and an exact-version consumer probe.
The current remote-controller tutorial must not redesign MicroWorld during a
lesson or move fail-closed product policy into the engine.

The roadmap includes core utilities, memory ownership, managed objects,
reflection-lite metadata, lifecycle/ticking, timers/subsystems, serialization,
network drivers/management, bounded replication, platform ports, diagnostics,
testing, examples, and documentation. It excludes full UE5 compatibility,
Blueprints/editor tooling, rendering, physics, audio, navigation, asset
cooking/streaming, GAS, a general task graph, unrestricted heap use, and
product-specific radio or safety policy. Display, input, and simple 2D
facilities remain future extension modules driven by real applications.

## Open Questions

- What exact reference boards and toolchains define the first three target
  ports? Recommended default: the existing ESP32-S3 N16R8 target, one
  STM32 development board selected by the owner, and one RP2040/RP2350 board
  selected by the owner.
- What are the minimum flash, RAM, stack, and per-update time budgets for the
  Core, Managed, and Connected profiles? These must be accepted before their
  implementations are optimized or called portable.
- Which concrete transport should be the first real `INetDriver` after the host
  loopback and bounded datagram/byte-stream fakes? E32 protocol and valve safety
  remain outside the generic engine.

## Decisions Log

- 2026-07-18: MicroWorld 0.1 was released as an independent, platform-neutral
  lifecycle/tick package with deterministic consumer ownership.
- 2026-07-19: The target vision expands MicroWorld into a lightweight mini
  engine for UE5 developers rather than only a lifecycle helper.
- 2026-07-19: The engine must target ESP32, STM32, RP2040-class boards, and
  comparable microcontrollers through portable core contracts and platform
  adapters.
- 2026-07-19: Managed memory/GC, NetManager/NetDriver concepts, smart pointers,
  and other essential UE-style facilities are required, while a full
  desktop-scale game engine remains out of scope.
- 2026-07-19: Select the layered UE-shaped, MCU-native architecture and keep
  module/profile boundaries revisable before 1.0 as measured evidence arrives.
- 2026-07-19: Make GC a first-class Managed-profile capability rather than a
  mandatory Core cost; deterministic hardware and safety paths stay outside GC.
- 2026-07-19: Target transferable UE5 concepts and selected honest type names,
  not source compatibility, and publish semantic differences explicitly.
- 2026-07-19: Use fixed arenas as the portable resource baseline while allowing
  explicitly configured heap-backed arenas on host and capable targets.
- 2026-07-19: Validate networking with host loopback and bounded transport
  fakes before selecting a real driver from an application requirement.
- 2026-07-19: Defer display/input helpers and repository extraction until
  measured foundations and multiple real consumers justify them.
