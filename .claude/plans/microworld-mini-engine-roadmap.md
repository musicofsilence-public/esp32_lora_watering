# MicroWorld Mini Engine Roadmap

This document defines milestone scope and acceptance checks. Current status,
next work, and evidence belong only in
[PROGRESS.md](../../lib/microworld/PROGRESS.md).

Historical detailed planning remains available at:

```text
git show 0c4aa73:.claude/plans/microworld-mini-engine-roadmap.md
```

## Foundation

- Core: deterministic lifecycle, tick scheduling, bounded registration, and
  explicit results.
- Memory: explicit resources, fixed storage, ownership helpers, containers,
  and delegates.
- Object: managed handles, roots, descriptors, fixed store, and bounded GC.

## 1. Minimal Engine

The milestone is limited to:

- `UWorld`, `AActor`, and `UActorComponent`;
- fixed-capacity registration before `BeginPlay`;
- World starts Actors in registration order;
- Actor starts Components before its own hook;
- Components tick before their Actor;
- deterministic reverse shutdown;
- traced downward ownership and weak parent links; and
- explicit duplicate, capacity, lifecycle, and cross-owner failures.

The application keeps one explicit root for `UWorld`; the World traces Actors,
Actors trace Components, and weak parent links prevent cycles.

Do not add dynamic spawn/destroy, timers, subsystems, network code, threads,
platform adapters, or hidden allocation in this milestone.

Acceptance checks:

- host behavior tests prove exact lifecycle/tick order and all rejection paths;
- GC retains World-owned Actors and Actor-owned Components, while weak parent
  links expire correctly;
- strict C++17 no-exceptions/no-RTTI consumer compiles;
- existing Core, Memory, and Object tests still pass.

## 2. Simple timers

Add a fixed-capacity Engine timer facility only after the minimal Engine passes
its acceptance checks.
It uses caller time, explicit capacity/handle failures, deterministic callback
order, cancellation, and no catch-up bursts.

Acceptance checks:

- tests cover one-shot, looping, cancellation, stale handles, capacity, and
  delayed updates;
- no timer path allocates or reads a hidden clock.

## 3. Simple Net

Create a small Net package with:

- bounded byte reader/writer;
- one non-blocking `INetDriver`;
- one small fixed-capacity `FNetManager`;
- explicit full, invalid, and unavailable results; and
- a host loopback implementation.

The application calls Engine and Net directly. Do not add sessions, transport
policy, replication, authentication, a bridge package, or a generic protocol
framework.

Acceptance checks:

- reader/writer bounds and malformed-input tests;
- loopback tests for bounded send/receive and backpressure;
- strict consumer compile and no hidden allocation in fixed-storage paths.

## 4. One ESP32-S3 example

Build one small example that demonstrates the completed runtime on the existing
ESP32-S3 configuration. It may compile without upload unless hardware execution
is explicitly authorized.

Acceptance checks:

- host example produces a deterministic trace;
- ESP32-S3 example compiles;
- documentation distinguishes compile evidence from hardware execution.

## Later, only when needed

Dynamic spawn/destroy, richer timers, real transports, serialization formats,
security, replication, hardware abstraction, additional boards, and engine
services enter only when a real application needs them.

## Working rules

- Implement the smallest usable milestone and verify its behavior.
- Keep portable code bounded, explicit, C++17, and free of product/platform
  policy.
- Update `PROGRESS.md` when status or evidence changes.
- Keep exact measurements in benchmark result records; do not turn compile
  success into a runtime or hardware claim.
