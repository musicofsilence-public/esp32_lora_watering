# MicroWorld Progress

This is the live status record for MicroWorld. Headers and tests define current
behavior; benchmark records contain measured facts.

## Current state

MicroWorld remains a small, bounded runtime. The minimal managed Engine, the
Simple Timers milestone, and the Simple Net milestone are accepted
implementation candidates; target runtime margins remain unmeasured.

| Package | State |
| --- | --- |
| Core | Released 0.1 lifecycle and tick package |
| Memory | Implemented candidate; target runtime margins unmeasured |
| Object / GC | Implemented candidate; target runtime margins unmeasured |
| Engine (incl. bounded timers) | Accepted implementation candidate; target runtime margins unmeasured |
| Net | Accepted implementation candidate; target runtime margins unmeasured |

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
  post-dispatch compaction that preserves multiple survivors and appends
  reused slots at the logical tail, cancellation, no catch-up bursts, and no
  observable steady-state allocation.
- Net: bounded `FByteWriter`/`FByteReader` over caller-owned `TSpan`, one
  non-blocking `INetDriver` with one bounded `TrySend`/`TryReceive`, one
  caller-storage-backed fixed-capacity `FNetManager<MaxPackets,
  MaxPacketBytes>` over a caller-supplied `FNetPacketStorage` with a
  deterministic outbound FIFO and one direct driver receive, normalized
  `Success`/`Full`/`Invalid`/`Unavailable` results, safe rejection of invalid
  `{nullptr, nonzero}` backing spans, transactional failure semantics, and a
  deterministic `FHostLoopback` driver. Net depends only on Core and Memory.

## Next

One ESP32-S3 example remains the next milestone: a small deterministic
demonstration of the completed runtime on the existing ESP32-S3 configuration.
It may compile without upload unless hardware execution is explicitly
authorized.

## Later

Real transports, wire framing, sessions, retries, reliability, authentication,
replication, additional boards, and richer engine services enter only when a
real application needs them.

## Evidence

| Area | Recorded evidence | Qualification |
| --- | --- | --- |
| Core | 31 behavior cases and 5 CTest checks | Released Core 0.1 evidence |
| Memory | 27 cases, including paired Clang 20 ASan/UBSan | Candidate evidence; target margins unmeasured |
| Object | 26 cases under MSVC Release, strict GCC 16, and paired Clang 20 ASan/UBSan | Candidate evidence; target margins unmeasured |
| Object ESP32 image | 20,172 bytes RAM and 198,877 bytes flash | Compile-only complete-image evidence |
| Minimal Engine | 21 lifecycle/registration/GC cases; four-package dependency check passed across 42 files | Accepted implementation candidate; target runtime margins unmeasured |
| Simple Timers | 34 timer behavior cases including mixed stable compaction and tail reuse; explicit mode allowlist, single-pass Advance compaction, and steady-state zero-allocation dispatch; strict Engine consumer built with exceptions and RTTI disabled exited 0; timer TU compiled clean under strict GCC 16 and Clang 19 warnings | Accepted implementation candidate; target runtime margins unmeasured |
| Engine ESP32 image | 20,332 / 327,680 bytes RAM (6.2%); 208,061 / 4,194,304 bytes flash (5.0%) | Compile-only complete-image evidence; +184 bytes flash from prior timer image |
| Simple Net | 52 byte/loopback/manager behavior cases including invalid-backing-span safety, valid empty `{nullptr, 0}` reader reporting an empty suffix view without pointer arithmetic, host-loopback null-destination-before-empty-queue `Invalid` rejection with sentinel-verified transactional outputs, private caller-owned `FNetPacketStorage` observed only through the matching `FNetManager` specialization, normalized ENetResult semantics, recorded-packet FIFO order with differently sized and valued packets, exact head retention across driver Full/Unavailable/Invalid, recovery sending the retained head before later packets, caller-storage reuse across wraparound, transactional receive failures, and steady-state zero-allocation; strict Core+Memory+Net consumer built with exceptions and RTTI disabled exited 0; Net TU compiled clean under strict GCC 16 and Clang 19 warnings; dependency/profile checkers updated and self-tested | Accepted implementation candidate; target runtime margins unmeasured |
| Net ESP32 image | 20,156 / 327,680 bytes RAM (6.2%); 196,773 / 4,194,304 bytes flash (4.7%) | Corrected-source compile-only complete-image evidence; clean retry passed after an earlier transient GCC 15.2.0 ICE in ESP-IDF vendor code |

No target upload, runtime timing, stack, heap, radio, or physical-hardware
claim has been recorded for Memory, Object, Engine, or Net.

- [Core host evidence](benchmarks/Results/Host.md)
- [Core ESP32 compile evidence](benchmarks/Results/Esp32S3N16R8.md)
- [Memory host evidence](../microworld-memory/benchmarks/Results/Host.md)
- [Memory ESP32 compile evidence](../microworld-memory/benchmarks/Results/Esp32S3N16R8.md)
- [Object host evidence](../microworld-object/benchmarks/Results/Host.md)
- [Object ESP32 compile evidence](../microworld-object/benchmarks/Results/Esp32S3N16R8.md)
- [Engine host evidence](../microworld-engine/benchmarks/Results/Host.md)
- [Engine ESP32 compile evidence](../microworld-engine/benchmarks/Results/Esp32S3N16R8.md)
- [Net host evidence](../microworld-net/benchmarks/Results/Host.md)
- [Net ESP32 compile evidence](../microworld-net/benchmarks/Results/Esp32S3N16R8.md)
- [Acceptance roadmap](../../.claude/plans/microworld-mini-engine-roadmap.md)
