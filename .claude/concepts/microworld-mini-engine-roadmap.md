# MicroWorld Mini Engine

MicroWorld is a small portable C++17 runtime that borrows a few useful UE-style
ideas without trying to reproduce UE. It is for bounded embedded applications,
not a desktop game engine.

## Current foundation

- **Core** provides deterministic lifecycle and tick dispatch with
  caller-supplied monotonic time.
- **Memory** provides explicit resources, fixed storage, bounded containers,
  delegates, and non-managed ownership helpers.
- **Object** provides managed identity, handles, roots, descriptors, and
  bounded incremental garbage collection.

Core is released. Memory and Object are implemented candidates; their target
runtime margins have not yet been measured. That qualification does not block
the next Engine implementation.

## First useful engine

The next milestone is a minimal managed Engine:

- `UWorld`, `AActor`, and `UActorComponent`;
- fixed-capacity registration before `BeginPlay`;
- deterministic `BeginPlay`, `Tick`, and `EndPlay` order;
- traced World-to-Actor and Actor-to-Component references;
- weak Actor-to-World and Component-to-Actor references; and
- explicit capacity and ownership failures.

The application holds one explicit root for `UWorld`. The World traces its
Actors and each Actor traces its Components, so the managed graph stays alive
through collection without parent ownership cycles.

The first version is intentionally static after play begins. Dynamic spawn and
destroy can wait until a real application needs them.

## Near-term sequence

After the minimal Engine works:

1. add simple fixed-capacity timers to Engine;
2. add simple Net: bounded byte reader/writer, one non-blocking `INetDriver`,
   one small fixed-capacity `FNetManager`, and a host loopback;
3. add one ESP32-S3 example that uses the runtime.

The application coordinates Engine and Net directly. There is no separate
Engine-Net bridge or subsystem framework in this first design.

## Permanent constraints

- C++17; no required exceptions or RTTI.
- Bounded storage and bounded work; capacity and invalid input failures are
  explicit.
- Scheduling time is supplied by the caller as monotonic time.
- Portable packages contain no platform SDK, hardware, or product policy.
- GC is optional. Hardware drivers, ISR state, watchdog paths, and safety
  services have deterministic ownership outside GC.
- No hidden allocation, hidden clock, background thread, or product-specific
  behavior.

## Modules

The first-version modules are Core, Memory, Object, Engine, and Net. Timers
belong to Engine. Net owns the bounded byte reader/writer, `INetDriver`, and
small fixed-capacity `FNetManager`.

Dependencies stay simple:

```text
Core <- Memory <- Object <- Engine
Core <- Memory <- Net
```

## Explicitly deferred

Do not create these until a real application proves the need:

- dynamic spawn/destroy and a subsystem framework;
- a separate Serialization package or Engine-Net bridge;
- replication, RPC, prediction, rollback, or a security framework;
- universal hardware abstraction, STM32/Pico support, or port programs;
- editor tooling, rendering, physics, audio, navigation, or asset systems.

The remote-controller project may consume MicroWorld later. Its radio and
fail-closed safety policy remain product code, not engine behavior.
