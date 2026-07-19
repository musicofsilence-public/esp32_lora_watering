# MicroWorld Progress

This is the live status record for MicroWorld. Headers and tests define current
behavior; benchmark records contain measured facts.

## Current state

MicroWorld remains a small, bounded runtime. The minimal managed Engine has
passed its acceptance checks; target runtime margins remain unmeasured.

| Package | State |
| --- | --- |
| Core | Released 0.1 lifecycle and tick package |
| Memory | Implemented candidate; target runtime margins unmeasured |
| Object / GC | Implemented candidate; target runtime margins unmeasured |
| Engine (incl. bounded timers) | Implemented candidate; target runtime margins unmeasured |
| Net | Next milestone |

## Visual roadmap

![MicroWorld implementation journey](docs/diagrams/microworld-implementation-roadmap.svg)

[Open the high-resolution PNG](docs/diagrams/microworld-implementation-roadmap.png)
or inspect the
[editable Mermaid source](docs/diagrams/microworld-implementation-roadmap.mmd).

## Done

- Core: bounded non-owning World/Actor/Component registration, deterministic
  lifecycle/tick scheduling, typed results, and caller-supplied time.
- Memory: explicit resources, fixed storage, ownership helpers, containers,
  and delegates.
- Object: generation-safe handles, descriptors, roots, fixed object storage,
  and bounded incremental collection.
- Engine: managed `UWorld`, `AActor`, and `UActorComponent`; fixed registration
  before play; deterministic lifecycle and tick order; traced downward
  ownership and weak parent links; explicit registration failures.
- Engine timers: bounded `TTimerManager<MaxTimers, InlineCallbackBytes>` with
  caller-supplied time, generation-checked handles local to the issuing
  manager, one-shot and looping scheduling (every other mode rejected
  transactionally), deterministic insertion-order dispatch with single-pass
  post-dispatch compaction, cancellation, no catch-up bursts, and no
  observable steady-state allocation.

## Next

Add a small bounded Net package: a byte reader/writer, one non-blocking
`INetDriver`, one fixed-capacity `FNetManager`, explicit full/invalid/
unavailable results, and a host loopback implementation.

## Later

One ESP32-S3 example remains a later milestone.

## Evidence

| Area | Recorded evidence | Qualification |
| --- | --- | --- |
| Core | 31 behavior cases and 5 CTest checks | Released Core 0.1 evidence |
| Memory | 27 cases, including paired Clang 20 ASan/UBSan | Candidate evidence; target margins unmeasured |
| Object | 26 cases under MSVC Release, strict GCC 16, and paired Clang 20 ASan/UBSan | Candidate evidence; target margins unmeasured |
| Object ESP32 image | 20,172 bytes RAM and 198,877 bytes flash | Compile-only complete-image evidence |
| Minimal Engine | 21 lifecycle/registration/GC cases; four-package dependency check passed across 42 files | Accepted candidate evidence; target runtime margins unmeasured |
| Simple Timers | 33 timer behavior cases; corrected mode rejection, single-pass Advance compaction, and steady-state zero-allocation dispatch; strict Engine consumer built with exceptions and RTTI disabled exited 0; timer TU compiled clean under strict GCC 16 warnings | Accepted candidate evidence; target runtime margins unmeasured |
| Engine ESP32 image | 20,332 / 327,680 bytes RAM (6.2%); 208,061 / 4,194,304 bytes flash (5.0%) | Compile-only complete-image evidence; +184 bytes flash from prior timer image |

No target upload, runtime timing, stack, heap, radio, or physical-hardware
claim has been recorded for Memory, Object, or Engine.

- [Core host evidence](benchmarks/Results/Host.md)
- [Core ESP32 compile evidence](benchmarks/Results/Esp32S3N16R8.md)
- [Memory host evidence](../microworld-memory/benchmarks/Results/Host.md)
- [Memory ESP32 compile evidence](../microworld-memory/benchmarks/Results/Esp32S3N16R8.md)
- [Object host evidence](../microworld-object/benchmarks/Results/Host.md)
- [Object ESP32 compile evidence](../microworld-object/benchmarks/Results/Esp32S3N16R8.md)
- [Engine host evidence](../microworld-engine/benchmarks/Results/Host.md)
- [Engine ESP32 compile evidence](../microworld-engine/benchmarks/Results/Esp32S3N16R8.md)
- [Acceptance roadmap](../../.claude/plans/microworld-mini-engine-roadmap.md)
