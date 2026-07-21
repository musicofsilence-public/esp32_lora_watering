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

- Core: deterministic lifecycle/tick scheduling primitives (`FApplication`,
  `FTickFunction`, `FLifecycleGuard`, `FTickable`), typed results, and
  caller-supplied time. The duplicate Core World/Actor/Component/Network model
  was retired in the Phase 1 consolidation so the managed Engine is the sole
  Actor model (see the Engine entry below).
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
| Roadmap Phase 0 baseline (host, 2026-07-20) | Windows MinGW-w64 UCRT g++ 16.1.0 via Ninja: Core 5/5, Memory 1/1, and Object 1/1 CTest suites pass; Engine and Net test executables fail to build (MinGW-UCRT libstdc++ has no `std::aligned_alloc`; one `-Werror=unused-variable`), while all five production libraries compile | Baseline recorded per MICROWORLD_ROADMAP.md task 0.1; root and lib `AGENTS.md` now register the roadmap as plan/tracker (task 0.2). The Engine/Net test-support portability gap is tracked in the roadmap as a downstream blocker for Phases 2 and 4; no fix attempted (baseline is record-only) |
| Roadmap Phase 1 consolidation (host, 2026-07-20) | Retired the duplicate Core actor model (`TWorld`/`TActor`/`FActorComponent`/`FNetwork`); the managed Engine `UWorld`/`AActor`/`UActorComponent` is the sole Actor model. GCC 16.1.0 via Ninja: `host-core` 5/5 and `host-eng` 1/1 CTest pass; the managed `HostLifecycle` example builds under `-fno-exceptions -fno-rtti` and prints its deterministic trace (exit 0); grep for `TWorld<`/`FActorBase`/`MicroWorld::FNetwork` over Core+Engine finds nothing; class-documentation (36 files) and Core dependency-boundary checkers pass; the retirement doc sweep leaves no doc presenting a retired type as current API. Core consumer probes retargeted to primitives and consolidated behind `CoreConsumerProbe.h` | Roadmap Phase 1 complete (tasks 1.1–1.4, per MICROWORLD_ROADMAP.md). Engine test-support `std::aligned_alloc` gap fixed for MinGW; the Net test-support copy remains a Phase 4 blocker. Strict `CheckFolderAgents.py` flags a pre-existing `docs/diagrams` guide gap unrelated to the retirement (roadmap section 6, proposed). ESP32 PlatformIO consumer builds not re-run here (Phase 5 gate) |
| Roadmap Phase 2 runtime spawn & destroy (host, 2026-07-21) | Runtime `UWorld::SpawnActor`/`DestroyActor` (queue-only, validated per the section-5 table) applied at one deferred barrier `ApplyPending(now)` — destroys first (guarded end-cascade, then unguarded component+actor `MarkPendingDestroy` + stable `RemoveAt`), then spawns (register + begin under a fresh guard); `PendingSpawnCount`/`PendingDestroyCount` observers; pending-spawn actors traced by `VisitReferences`; bounded pending storage added to `FWorldActorRegistry`. Ergonomic `TInlineActor<N>`/`TInlineWorld<N>` (base-from-member inline registry). GCC 16.1.0 via Ninja: `host-eng` builds clean under `-Wall -Wextra -Wpedantic -Werror -fno-exceptions -fno-rtti`; CTest 1/1; runner 69 host cases, 0 failures (14 new across spawn/destroy + inline types); `HostLifecycle` example rewritten on inline types runs (exit 0) with the unchanged deterministic trace; `CheckClassDocumentation.py --root lib/microworld-engine` passes (24 files) | Roadmap Phase 2 complete (tasks 2.1–2.4, per MICROWORLD_ROADMAP.md); target runtime margins unmeasured. Resolved item for Phase 3 (roadmap section 6, 2026-07-21, option a): the object-store GC sweep skips pending-destroy slots, so destroyed actors are reclaimed by the store's `ApplyPendingDestroy` barrier, not GC mark/sweep — the section-4 frame order gained an explicit bounded reclamation slice before the GC slice, to be wired into `TEngineHost` in Phase 3.2. ESP32 image not re-measured here (Phase 5 gate) |
| Roadmap Phase 3 composition root & logging (host, 2026-07-21) | Accepted implementation candidate; target runtime margins unmeasured. `MW_LOG` bounded logging facade (`lib/microworld/include/MicroWorld/Log.h` + `src/Log.cpp`, task 3.1). `TEngineHost` composition template wiring class registry, object store, garbage collector, world actor registry, and timer manager behind one canonical per-frame order — timers, world advance, pending spawn/destroy barrier, bounded `Store.ApplyPendingDestroy` reclamation slice, then idle-gated bounded GC slice — with a transactional non-monotonic-time guard (task 3.2). Five `TEngineHost` behavior cases in `lib/microworld-engine/tests/EngineHostTests.cpp` (lifecycle begin/tick/end order; timer-before-tick frame order; bounded GC reclamation of unrooted objects across ticks; transactional non-monotonic-time rejection; idempotent `EndPlay`), wired into `MICROWORLD_ENGINE_TEST_SOURCES`. `HostLifecycle` example rewritten on `TEngineHost` (the `FDeviceWorld`/`TInlineWorld` typedef and hand-rolled store/registry/world composition are gone); canonical lifecycle trace unchanged. Applied the 3.2 gap fix: public `TEngineHost::FindClass(FTypeId)` so user descriptors reference the registry's own parent copies and user types construct through them. GCC 16.1.0 via Ninja: `host-eng` builds clean under `-Wall -Wextra -Wpedantic -Werror -fno-exceptions -fno-rtti`; CTest 1/1; runner 74 host cases, 0 failures (5 new); `microworld_engine_host_lifecycle` prints the unchanged lifecycle trace and exits 0; `CheckClassDocumentation.py --root lib/microworld-engine` passes (24 files). Task 3.4 added `TEngineHost::RegisterClass<T>(FTypeId, const char*)` (derives the parent from `T`'s engine base, builds the descriptor with the shared managed tracer) and `CreateObject<T>(FTypeId, Args&&...)` (folds `FindClass` + `NewObject`; `UnknownClass` + null for an unregistered id), with two new host cases (helper-driven register/construct/lifecycle; unregistered-id rejection) and the `HostLifecycle` example simplified onto them (canonical trace byte-identical; `main()` 46 → 43 lines, register+construct section 18 → 7 lines); runner now 78 host cases, 0 failures (2 new) | Roadmap Phase 3 complete (tasks 3.1–3.4, per MICROWORLD_ROADMAP.md); target runtime margins unmeasured. Resolved item for Phase 3 (roadmap section 6, 2026-07-21, option a): `TEngineHost` exposed `RegisterClass` but no lookup, and the object store validates descriptor identity against the registry's own copy — added the minimal `FindClass` accessor rather than an auto-register-and-construct helper. ESP32 image not re-measured here (Phase 5 gate) |

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
