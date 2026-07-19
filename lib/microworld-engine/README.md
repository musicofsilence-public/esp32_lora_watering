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
- Lifecycle methods return `ERuntimeResult`; registration methods return
  `EEngineResult`.

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

Engine does not provide timers, networking, runtime spawn/destroy, subsystems,
serialization, replication, platform abstraction, or hardware APIs.
