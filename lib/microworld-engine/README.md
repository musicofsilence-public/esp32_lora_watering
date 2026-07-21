# MicroWorld Engine

MicroWorld Engine is an implementation candidate for the managed-runtime layer
above Object. It provides `UWorld`, `AActor`, and `UActorComponent` for bounded
embedded applications. The candidate has passed its acceptance checks; target
runtime margins remain unmeasured.

Current status and recorded evidence live in
[PROGRESS.md](../microworld/PROGRESS.md).

## What Engine provides

- `UWorld` registers and dispatches `AActor` instances.
- `AActor` registers and dispatches `UActorComponent` instances.
- Registration is fixed-capacity and closes at `BeginPlay`.
- Begin and tick use registration order; shutdown uses reverse registration
  order. Components begin and tick before their Actor; Actors end before their
  Components.
- `UWorld` traces Actors; `AActor` traces Components; parent links are weak and
  expire when the parent is reclaimed.
- Caller-supplied monotonic milliseconds drive scheduling.
- `TTimerManager<MaxTimers, InlineCallbackBytes>` schedules fixed-capacity
  one-shot and looping timers from caller-supplied time. Every other
  `ETimerMode` value (including `None` and arbitrary casts) is rejected
  transactionally as `InvalidMode`. The application owns the manager value,
  supplies every clock reading, and decides when Advance is called relative to
  World dispatch. `FTimerHandle` is a {slot index, generation} pair local to
  the issuing manager; completed one-shots are cleared in place and removed in
  a single stable post-dispatch compaction pass.
- Lifecycle methods return `ERuntimeResult`; registration methods return
  `EEngineResult`; timer methods return `ETimerResult`.

The application owns the object store, root table, GC worklist, caller-owned
registration storage, and one `TStrongObjectPtr<UWorld>` root.

## Build

```sh
cmake -S lib/microworld-engine -B <build-directory>
cmake --build <build-directory>
ctest --test-dir <build-directory> --output-on-failure
```

CMake consumers link `MicroWorld::Engine`. A successful compile or host test
does not establish target runtime margins or hardware behavior.

## Example

[`examples/HostLifecycle/Main.cpp`](examples/HostLifecycle/Main.cpp) is the
canonical "hello MicroWorld": it builds a managed composition through
`TEngineHost`, registers one actor and one ticking component, and drives
`BeginPlay` / `Tick` / `EndPlay` so the deterministic lifecycle order is visible
on stdout. Build it with the CMake target `microworld_engine_host_lifecycle`
(included by the default `host-eng` configuration above). It is the smallest
demonstration of the composition root added in roadmap Phase 3.

Engine does not provide networking, runtime spawn/destroy, subsystems,
serialization, replication, platform abstraction, or hardware APIs.
