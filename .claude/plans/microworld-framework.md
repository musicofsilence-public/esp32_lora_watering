# MicroWorld Core 0.1 Historical Release Record

| Field | Value |
| --- | --- |
| Created | 2026-07-18 |
| Completed | 2026-07-18 |
| Status | Complete — MicroWorld Core 0.1.0 released |
| Release anchor | Commit `c54f3c4` |
| Scope | Standalone deterministic lifecycle and primary-tick Core package |
| Live status | [MicroWorld PROGRESS](../../lib/microworld/PROGRESS.md) |
| Original plan archive | `git show cf5d964:.claude/plans/microworld-framework.md` |

> **Historical record.** This document records the completed v0.1 release. It
> is not an active implementation plan. The archive anchor preserves the full
> original proposal, including its sketches, checklists, and task breakdown.

## Authority

- [README](../../lib/microworld/README.md), public headers, behavior tests, and
  [CHANGELOG](../../lib/microworld/CHANGELOG.md) define the released Core 0.1
  contract.
- [Host evidence](../../lib/microworld/benchmarks/Results/Host.md) and
  [ESP32-S3 evidence](../../lib/microworld/benchmarks/Results/Esp32S3N16R8.md)
  own their exact source, environment, toolchain, and measurement claims.
- [PROGRESS](../../lib/microworld/PROGRESS.md) is the sole live source for
  post-0.1 implementation, gate, decision, blocker, and next-milestone state.
- This record owns only the completed v0.1 objective, decisions, and handoff.

## Original objective and scope

Separate framework development from the ESP32 teaching sequence by delivering
an independently buildable C++17 package under `lib/microworld`. The package
had to preserve bounded ownership, deterministic lifecycle and ticking, and a
platform-neutral dependency boundary without importing ESP-IDF, FreeRTOS,
Arduino, E32, or remote-controller product policy.

The v0.1 scope was deliberately small:

- one consumer-owned application composition root;
- fixed-capacity, non-owning World/Actor/Component registration;
- guarded forward-only lifecycle;
- independently configured primary ticks;
- one policy-free Network lifecycle/tick boundary;
- host tests, compile consumers, fixed benchmarks, documentation, and package
  metadata.

Reflection, garbage collection, dynamic spawning, transforms, rendering,
protocol policy, platform adapters, task graphs, and unrestricted allocation
were outside this release.

## Released contract

- `FApplication` guards the consumer composition root and propagates terminal
  begin failures.
- `TWorld<MaxActors>` dispatches a bounded set of non-owning Actor
  registrations in deterministic order.
- `TActor<MaxComponents>` owns no Component storage and dispatches bounded
  registrations.
- `FActorComponent` provides a focused lifecycle and independent primary-tick
  boundary.
- `FNetwork` is a policy-free lifecycle/tick hook, not a transport or network
  manager.
- `FTickConfiguration`, `FTickContext`, and `FTickFunction` implement
  independent enablement, interval-zero updates, monotonic validation,
  per-object elapsed time, saturation, and no catch-up bursts.
- Capacity, duplicate registration, ownership, lifecycle, and time failures
  are explicit results; lifecycle and tick paths perform no framework heap
  allocation.

## Durable decisions

- Keep MicroWorld independently buildable in this repository until multiple
  consumers justify extraction.
- Require C++17 without reliance on exceptions or RTTI.
- Keep concrete ownership in the consumer composition root; registrations are
  bounded, non-owning, single-owner, and pointer-stable.
- Components tick before their Actor; Actor and Component tick enablement
  remain independent.
- A late tick executes at most once and schedules from actual execution time.
- Only dispatcher `Advance` calls supply canonical monotonic time.
- Runtime objects are non-copyable and non-movable; structural registration
  closes when play begins.
- Use honest UE-inspired `F`/`T`/`E`/`b` naming without implying UObject
  semantics.
- Measure before optimizing; retain readable control flow unless target
  evidence justifies a change.
- Keep product safety, hardware, transport, and protocol policy outside Core.

## Delivered artifacts

- Portable public headers and source under `lib/microworld`.
- Standalone CMake/CTest build and `MicroWorld::Core` target compatibility.
- PlatformIO package metadata and exact-version native/ESP32-S3 consumers.
- Behavior tests for lifecycle, ownership, registration, capacity, scheduling,
  timing, Application, and Network boundaries.
- Fixed host and ESP32-S3 benchmark consumers and evidence records.
- Host lifecycle example, API/porting/performance/style documentation, release
  metadata, and scoped contributor guides.

## Verification and evidence

The released Core evidence records:

- 31 host behavior cases;
- strict public-header and C++17 consumer compilation;
- exceptions-disabled and RTTI-disabled consumption;
- class-documentation, folder-guide, dependency, and package checks;
- standalone CMake and PlatformIO native consumption;
- exact-version ESP32-S3 basic and benchmark compile consumers;
- fixed host workload and object-size observations.

Use the exact records rather than copying measurements here:

- [Core host result](../../lib/microworld/benchmarks/Results/Host.md)
- [Core ESP32-S3 compile result](../../lib/microworld/benchmarks/Results/Esp32S3N16R8.md)
- [Released contract](../../lib/microworld/README.md)
- [0.1.0 release notes](../../lib/microworld/CHANGELOG.md)

No firmware was uploaded or run for the release evidence. ESP32-S3 runtime
cycles, heap, stack, timing, electrical behavior, and hardware operation were
not inferred from compile success.

## Released limitations

Core 0.1 has no runtime registration/removal, dynamic spawning, lookup,
reflection, garbage collection, event bus, transforms, protocol policy, tick
groups/prerequisites, parallel ticking, or real-time guarantee. Capacities are
selected by the consumer at compile time. Source compatibility before 1.0 is
not promised.

## Post-0.1 relationship

The durable post-0.1 architecture is in the
[mini-engine concept](../concepts/microworld-mini-engine-roadmap.md), and its
ordered gates are in the
[execution roadmap](microworld-mini-engine-roadmap.md). Those documents may
add separately linkable candidates, but they do not rewrite or silently extend
the released Core 0.1 contract.

## Immutable completion history

| Date | Record |
| --- | --- |
| 2026-07-18 | Project owner separated MicroWorld implementation from the ESP32 tutorial. |
| 2026-07-18 | Project owner required independent Actor and Component ticking. |
| 2026-07-18 | The same-repository, independently buildable Core package boundary was selected. |
| 2026-07-18 | Review clarified canonical dispatcher time, single registration, pointer stability, and exact-version consumers. |
| 2026-07-18 | Review required full source anchors, measured optimization, honest UE-style naming, concise class contracts, and scoped contributor guides. |
| 2026-07-18 | MicroWorld Core 0.1.0 was released at `c54f3c4`. |
