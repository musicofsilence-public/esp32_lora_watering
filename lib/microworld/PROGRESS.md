# MicroWorld Progress

This is the live status record for MicroWorld. Headers and tests define current
behavior; benchmark records contain measured facts.

## Current state

On 2026-07-19, the owner directed that MicroWorld remain a simple, functional
mini engine. Development proceeds from the existing foundation to the minimal
Engine; no extra approval gate or design document is required first.

| Package | State |
| --- | --- |
| Core | Released 0.1 lifecycle and tick package |
| Memory | Implemented candidate; target runtime margins unmeasured |
| Object / GC | Implemented candidate; target runtime margins unmeasured |
| Engine | Next implementation milestone |
| Net | Later, after the simple Engine and timers |

## Done

- Core: bounded non-owning World/Actor/Component registration, deterministic
  lifecycle/tick scheduling, typed results, and caller-supplied time.
- Memory: explicit resources, fixed storage, ownership helpers, containers,
  and delegates.
- Object: generation-safe handles, descriptors, roots, fixed object storage,
  and bounded incremental collection.

## Next

Implement the minimal managed Engine: `UWorld`, `AActor`, and
`UActorComponent`; fixed registration before play; deterministic lifecycle and
tick order; traced child references; weak parent references; and explicit
capacity/ownership failures.

## Later

Add simple fixed-capacity timers, then simple Net: bounded byte reader/writer,
one non-blocking `INetDriver`, one small fixed-capacity `FNetManager`, and host
loopback. Then add one ESP32-S3 example. Dynamic spawning, replication,
platform abstraction, and other expansion wait for a real application need.

## Evidence

| Area | Recorded evidence | Qualification |
| --- | --- | --- |
| Core | 31 behavior cases and 5 CTest checks | Released Core 0.1 evidence |
| Memory | 27 cases, including paired Clang 20 ASan/UBSan | Candidate evidence; target margins unmeasured |
| Object | 25 cases under MSVC Release, strict GCC 16, and paired Clang 20 ASan/UBSan | Candidate evidence; target margins unmeasured |
| Object ESP32 image | 20,172 bytes RAM and 198,877 bytes flash | Compile-only complete-image evidence |

No target upload, runtime timing, stack, heap, radio, or physical-hardware
claim has been recorded for Memory or Object.

- [Core host evidence](benchmarks/Results/Host.md)
- [Core ESP32 compile evidence](benchmarks/Results/Esp32S3N16R8.md)
- [Memory host evidence](../microworld-memory/benchmarks/Results/Host.md)
- [Memory ESP32 compile evidence](../microworld-memory/benchmarks/Results/Esp32S3N16R8.md)
- [Object host evidence](../microworld-object/benchmarks/Results/Host.md)
- [Object ESP32 compile evidence](../microworld-object/benchmarks/Results/Esp32S3N16R8.md)
- [Active concept](../../.claude/concepts/microworld-mini-engine-roadmap.md)
  and [roadmap](../../.claude/plans/microworld-mini-engine-roadmap.md)
