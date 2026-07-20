# MicroWorld — Review, Refactor & Implementation Plan

**Version:** 1.1 · **Date:** 2026-07-20 · **Owner:** Mykola
**Scope:** the MicroWorld package family under `lib/` (`microworld`, `microworld-memory`, `microworld-object`, `microworld-engine`, `microworld-net`).

This document is the **master improvement plan and progress tracker** for turning the
current MicroWorld candidate into a production-ready micro engine with UE5-like
concepts. It is written so that any LLM (including a weak one) can pick it up,
find the next task, complete it, and record progress without extra context.

`lib/microworld/PROGRESS.md` remains the evidence record (test counts, image
sizes, benchmark facts). **This file tracks the plan; PROGRESS.md tracks the
proof.** When a phase is accepted, update both.

---

## 1. How to use this document (protocol for LLM workers)

Follow these rules exactly:

1. Read section **2 (Ground rules)** before touching any code.
2. Open section **7 (Progress tracker)**. Find the first phase whose status is
   not ✅. Inside that phase, find the first unchecked `[ ]` task.
3. Work on **exactly one task at a time**. Do not start a later phase while an
   earlier phase has unchecked tasks. Tasks inside one phase must be done in
   order unless marked `(parallel-ok)`.
4. Every task has a **Done when** checklist and a **Verify** command block.
   A task is complete only when every "Done when" item is true and every
   "Verify" command passes.
5. When a task is complete: change its `[ ]` to `[x]` in this file, and update
   the phase status in the tracker table (⬜ → 🟨 when the first task of a phase
   starts, 🟨 → ✅ when the last task finishes).
6. When a phase reaches ✅: add one short evidence entry to
   `lib/microworld/PROGRESS.md` (what was built, how it was verified).
7. If you are blocked, mark the task `⛔ BLOCKED:` with one sentence explaining
   why, directly under the task title, and stop. Do not skip ahead.
8. Never delete or rewrite this document's structure. Only update statuses,
   checkboxes, and BLOCKED notes.

Status legend: ⬜ not started · 🟨 in progress · ✅ done · ⛔ blocked

---

## 2. Ground rules (invariants — never violate)

These apply to every task in this plan. They come from `lib/AGENTS.md` and the
existing code style, and they are what makes MicroWorld embedded-suitable.

- **C++17**, builds with exceptions and RTTI **disabled**. No `throw`, no
  `dynamic_cast`, no `typeid`.
- **No hidden allocation.** Steady-state code paths never call `new`/`malloc`.
  All storage is caller-owned, fixed-capacity, and compile-time bounded
  (`std::array`, template capacity parameters). Startup-time allocation is
  allowed only inside explicit `IMemoryResource` implementations.
- **No hidden clock.** Time is always caller-supplied
  `TimePointMilliseconds` (u64 ms). Only platform adapter packages may read
  real clocks.
- **Errors are enums, not exceptions.** Every fallible operation returns a
  result enum (`ERuntimeResult`, `EObjectResult`, `EEngineResult`,
  `ETimerResult`, `ENetResult`, `EDelegateResult`). Failures are
  **transactional**: a failed call leaves all inputs and state unchanged.
- **Determinism.** Registration order defines dispatch order. Begin/tick run in
  registration order; shutdown runs in reverse order. Components begin and tick
  before their Actor; Actors end before their Components. No catch-up bursts:
  a late tick runs once.
- **Naming:** `F` plain class/struct, `T` template, `E` enum, `I` interface,
  `b` bool prefix, `U`/`A` reserved for managed (`UObject`-derived) types.
- **Every function declaration and persistent member gets a doc comment**
  explaining intent/invariant/ownership (enforced by
  `lib/microworld/tools/CheckClassDocumentation.py`).
- **Format** with `clang-format --style=file:clang-format` (repo root policy
  file).
- **Dependency direction** (must stay acyclic; enforced by
  `lib/microworld/tools/CheckDependencyBoundaries.py`):

```
microworld-memory  →  (nothing)
microworld (Core)  →  (nothing)
microworld-object  →  Core, Memory
microworld-engine  →  Core, Memory, Object
microworld-net     →  Core, Memory
platform adapters  →  anything above, never the reverse
```

- **Every new public behavior needs host tests** in the owning package's
  `tests/` directory, wired into its CMake + CTest.
- Building never flashes hardware. Uploading/running on a board requires
  explicit human authorization (see root `AGENTS.md` safety rules).

---

## 3. Review verdict — what exists today

Reviewed at commit state of 2026-07-20. Quality is generally **high**: strict
warnings, transactional failure semantics, generation-checked handles, real
test suites, zero steady-state allocation. The problems are **duplication**,
**missing must-have features**, and **no platform layer** — not code quality.

### 3.0 Codebase map (navigation reference for workers)

Every public type and where it lives. Consult this before searching.

**`lib/microworld` (Core) — `include/MicroWorld/`**

| File | Types | Purpose |
| --- | --- | --- |
| `Time.h` | `TimePointMilliseconds` (u64), `DurationMilliseconds` (u32), `FTickContext`, `ERuntimeResult`, `FTickDecision` | Canonical time + shared result enum |
| `Lifecycle.h` | `ELifecycleState`, `FLifecycleGuard` | Forward-only lifecycle state machine (Constructed→Playing→Ended, Failed terminal) |
| `TickFunction.h` / `src/TickFunction.cpp` | `FTickConfiguration`, `FTickFunction` | One bounded per-object schedule; monotonic validation; at most one due tick |
| `Tickable.h` | `FTickable` | Mix-in giving a type one primary tick |
| `Application.h` / `src/Application.cpp` | `FApplication` | Abstract composition root (BeginPlay/Advance/EndPlay hooks) |
| `Version.h` | version constants | |
| `World.h`, `Actor.h`, `ActorComponent.h`, `Network.h` + `.cpp` | `TWorld`, `FActorBase`, `TActor`, `FActorComponent`, `FNetwork` | **RETIRED in Phase 1 — do not build on these** |

Tests: `tests/TickFunctionTests.cpp` (keep), `WorldTests.cpp` + `ApplicationNetworkTests.cpp` (retire in 1.3). Checker scripts: `tools/*.py`.

**`lib/microworld-memory` — `include/MicroWorld/`**

| File | Types | Purpose |
| --- | --- | --- |
| `Containers/Span.h` | `TSpan<T>` | Non-owning pointer+size view |
| `Containers/StaticVector.h` | `TStaticVector<T,N>` | Fixed-capacity vector, no heap |
| `Delegates/Delegate.h` | `TDelegate<Sig,InlineBytes>`, `TMulticastDelegate<Sig,MaxBindings,InlineBytes>`, `FDelegateHandle`, `EDelegateResult` | Bounded inline-storage callbacks + events |
| `Memory/MemoryResource.h` / `src/…cpp` | `IMemoryResource`, `FMemoryBlock` | Explicit allocation interface |
| `Memory/FixedArena.h` | `TFixedArena<Bytes>` | Caller-owned arena resource |
| `Memory/UniquePtr.h` | `TUniquePtr<T>` | Resource-bound unique ownership |
| `Memory/SharedPtr.h` | `TSharedPtr<T>`, `TWeakPtr<T>`, `MakeShared` | Resource-bound shared ownership |

**`lib/microworld-object` — `include/MicroWorld/Object/`**

| File | Types | Purpose |
| --- | --- | --- |
| `ObjectHandle.h` | `FObjectHandle` {index,generation}, `EObjectResult`, `FObjectId` | Generation-safe local identity |
| `ClassDescriptor.h` | `FClassDescriptor`, `TClassRegistry<MaxClasses>`, type tokens | No-RTTI type identity, ancestry, exact destruction |
| `Object.h` | `UObject` | Managed base: store identity, `VisitReferences`, `BeginDestroy` |
| `ObjectPtr.h` | `TObjectPtr<T>` (traced), `TWeakObjectPtr<T>` (observing), `TStrongObjectPtr<T>` (explicit root) | The three managed reference kinds |
| `ObjectStore.h` / `src/…cpp` | `FObjectStore`, `FObjectStoreStorage`, `TObjectCreationResult<T>`, `NewObject<T>`, `Resolve`, `MarkPendingDestroy`, roots API, dispatch guard | Fixed-slot object store |
| `GarbageCollector.h` / `src/…cpp` | `FGarbageCollector`, `FGarbageCollectionBudget/Result/Stats`, `EGarbageCollectionPhase`, `FReferenceCollector` | Budgeted incremental mark/sweep: `RequestCollection()` → `Advance(Budget)`; `CollectFull()`, `CancelCollection()` |

**`lib/microworld-engine` — `include/MicroWorld/Engine/`**

| File | Types | Purpose |
| --- | --- | --- |
| `EngineResult.h` | `EEngineResult` | Registration outcomes (adds CrossStore, InvalidReference) |
| `EngineClassIds.h` | `UWorldClassId`, `AActorClassId`, `UActorComponentClassId` | Stable type ids for base descriptors |
| `EngineStorage.h` | `FActorComponentRegistry<N>`, `FWorldActorRegistry<N>` | Caller-owned registry storage, one-shot `MakeView()` lease |
| `EngineRegistryView.h` | `FActorComponentRegistryBase`, `FWorldActorRegistryBase` | Move-only leases; only AActor/UWorld may use them |
| `World.h` / `src/World.cpp` | `UWorld` | Managed world; traces actors; gains Spawn/Destroy in Phase 2 |
| `Actor.h` / `src/Actor.cpp` | `AActor` | Managed actor; traces components; weak world parent |
| `ActorComponent.h` / `src/…cpp` | `UActorComponent` | Managed component; weak actor parent |
| `Timer.h` | `TTimerManager<MaxTimers,InlineCallbackBytes>`, `FTimerHandle`, `ETimerMode` (OneShot/Looping only), `ETimerResult` | Bounded timers, caller time |

Tests: `EngineLifecycleTests`, `EngineRegistrationTests`, `EngineGarbageCollectionTests`, `EngineTimerManagerTests`, `EngineAllocationCounters`.

**`lib/microworld-net` — `include/MicroWorld/Net/`**

| File | Types | Purpose |
| --- | --- | --- |
| `NetResult.h` | `ENetResult` {Success, Full, Invalid, Unavailable} | Normalized: Full = no capacity now; Invalid = never succeeds; Unavailable = retry later |
| `ByteWriter.h` / `ByteReader.h` | `FByteWriter`, `FByteReader` | Bounded LE serialization over caller spans |
| `NetDriver.h` / `src/…cpp` | `INetDriver`, `FNetReceiveResult` | Non-blocking transport; gains addressing in Phase 4 |
| `NetPacketStorage.h` | `FNetPacketStorage<MaxPackets,MaxPacketBytes>` | Caller-owned FIFO backing |
| `NetManager.h` | `FNetManager<MaxPackets,MaxPacketBytes>` | Outbound FIFO (head retained on failure) + direct receive |
| `HostLoopback.h` | `FHostLoopback` | Deterministic in-process driver for tests |

### 3.1 KEEP AS-IS (good, do not rewrite)

| Area | What | Why it stays |
| --- | --- | --- |
| Core primitives | `FTickFunction`, `FTickable`, `FLifecycleGuard`, `FApplication`, `Time.h` (`FTickContext`, `ERuntimeResult`) | Clean, tested, the foundation everything else already uses. Monotonic-time validation and forward-only lifecycle are exactly right for MCUs. |
| Memory | `TFixedArena`, `IMemoryResource`, `TUniquePtr`, `TSharedPtr`/`TWeakPtr`, `TStaticVector`, `TSpan`, `TDelegate`, `TMulticastDelegate` | Complete, bounded, ASan/UBSan-tested. Multicast delegates already exist — reuse them for engine events, do not invent a second event system. |
| Object / GC | `UObject`, `FObjectHandle` (index+generation), `FClassDescriptor` + `TClassRegistry` (no-RTTI type identity), `FObjectStore` (caller-owned slots), `TObjectPtr`/`TWeakObjectPtr`/`TStrongObjectPtr`, bounded incremental `FGarbageCollector` | This is the hardest part of a mini-UE and it is already done well: generation-safe handles, budgeted mark/sweep, no reflection, no RTTI, caller-owned storage. |
| Engine | `UWorld`, `AActor`, `UActorComponent`, `TTimerManager` | Correct managed layer; matches UE5 mental model. Gains spawn/destroy in Phase 2 but the lifecycle/dispatch core is sound. |
| Net building blocks | `FByteWriter`/`FByteReader`, `ENetResult`, `FNetManager` FIFO discipline, `FHostLoopback` | Solid bounded serialization and queue semantics. Extended (not replaced) in Phase 4. |
| Process | AGENTS.md guides, doc-comment rule, checker scripts in `lib/microworld/tools/`, per-package tests/benchmarks/Results structure | This discipline is why the code is trustworthy. Keep enforcing it. |

### 3.2 REFACTOR (works, but wrong shape for the goal)

| # | Problem | Decision |
| --- | --- | --- |
| R1 | **Two parallel Actor models.** Core's `TWorld`/`TActor`/`FActorBase`/`FActorComponent` duplicate Engine's `UWorld`/`AActor`/`UActorComponent`. Two ways to do everything = double docs, double tests, permanent confusion. | **Retire the Core actor model** (Phase 1). Engine layer becomes the only user-facing World/Actor/Component API. Core keeps only shared primitives. (Owner decision, 2026-07-20.) |
| R2 | **`FNetwork` in Core** (`microworld/include/MicroWorld/Network.h`) is a second, unrelated "network" concept that overlaps `microworld-net`. | Delete together with R1 (Phase 1). `microworld-net` is the only networking layer. |
| R3 | **Registry-lease ceremony.** Creating one actor requires a separate caller-owned `FActorComponentRegistry<N>` object plus a `MakeView()` lease passed into the constructor. Correct, but verbose and hostile to runtime spawning. | Keep the lease mechanism internally; add `TInlineActor<N>` / `TInlineWorld<N>` convenience templates that own their registry storage (Phase 2). |
| R4 | **Registration frozen at BeginPlay.** No runtime spawn/destroy — a static framework, not an engine. | Implement bounded deferred spawn/destroy (Phase 2). (Owner decision: must-have.) |
| R5 | **`INetDriver` has no addressing.** It models one point-to-point link, so a server can never tell peers apart — client/server roles are impossible on top of it. | Extend driver interface with an opaque `FNetAddress` (Phase 4). Breaking change, accepted while consumers are few. |
| R6 | **No composition recipe.** Wiring store + registry + roots + GC budget + timers + world takes ~60 lines of expert boilerplate in every app. | Add one `TEngineHost` composition template with a canonical frame order (Phase 3). |
| R7 | Docs describe the retired split (`UE5ConceptMap.md`, READMEs, `PROGRESS.md`). | Refresh at the end of each phase; final sweep in Phase 6. |

### 3.3 IMPLEMENT (missing must-haves)

| # | Missing | Delivered by |
| --- | --- | --- |
| M1 | Runtime `SpawnActor` / `DestroyActor` with deterministic deferred application | Phase 2 |
| M2 | One-call engine composition (`TEngineHost`) with canonical frame order incl. GC budget slot | Phase 3 |
| M3 | Minimal logging facade (`MW_LOG`) with compile-time level stripping | Phase 3 |
| M4 | Network roles: `ENetMode` (Standalone / Client / ListenServer / DedicatedServer), bounded peer table, hello/heartbeat/timeout session control, channel-based message send/receive. **Simple messages, not UE replication.** | Phase 4 |
| M5 | Real transports: UDP driver (host + ESP32) first, E32 LoRa UART driver second | Phase 5 |
| M6 | Platform adapter package for ESP32 (time source, app_main glue) | Phase 5 |
| M7 | End-to-end examples (ESP32 device app; two-node client/server demo) + measured runtime margins | Phase 6 |

### 3.4 Explicitly OUT OF SCOPE (do not build, even if asked nicely)

Rendering, physics, audio, assets, editor tooling, reflection generation,
UE-style property replication/RPC, threads/task graph, tick groups,
runtime component add/remove on live actors (components are fixed at spawn),
universal hardware APIs. If a future need appears, it gets its own plan first.

---

## 4. Target architecture (after this plan)

```
┌─────────────────────────────────────────────────────────────┐
│ Application (user code)                                     │
│   TEngineHost<...> owns: ClassRegistry, ObjectStore storage,│
│   GC + budget, UWorld root, TTimerManager, TNetHost         │
└──────────────┬──────────────────────────────────────────────┘
               │ Tick(nowMs) — one canonical frame
┌──────────────▼──────────────┐  ┌───────────────────────────┐
│ microworld-engine           │  │ microworld-net            │
│ UWorld / AActor /           │  │ TNetHost (roles, peers,   │
│ UActorComponent,            │  │ channels) over INetDriver │
│ Spawn/Destroy, TTimerManager│  │ ByteWriter/Reader         │
└──────┬──────────┬───────────┘  └─────────────┬─────────────┘
┌──────▼────┐ ┌───▼──────────────┐             │
│ microworld│ │ microworld-object│             │
│ (Core:    │ │ UObject, store,  │             │
│ tick/life-│ │ handles, GC      │             │
│ cycle/time│ └───┬──────────────┘             │
└──────┬────┘     │      ┌─────────────────────▼───┐
┌──────▼──────────▼──┐   │ platform adapters       │
│ microworld-memory  │   │ esp32: time, UDP, LoRa  │
│ arena, ptrs, spans,│   │ host: time, UDP,        │
│ delegates          │   │ loopback                │
└────────────────────┘   └─────────────────────────┘
```

Frame order inside `TEngineHost::Tick(now)` (fixed, documented, tested):

1. `NetHost.PumpReceive(now)` — drain driver, dispatch messages, update peers
2. `Timers.Advance(now)` — fire due timer callbacks
3. `World.Advance(now)` — components tick, then actors
4. `World.ApplyDeferred(now)` — pending spawns begin play; pending destroys end play + release
5. GC slice — `RequestCollection()` when idle (policy: every tick), then `Advance(Budget)`
6. `NetHost.PumpSend(now)` — flush outbound FIFO, heartbeats

---

## 5. Phases and tasks

### Phase 0 — Baseline & governance ⬜

Goal: prove the current state builds and passes everywhere before changing it,
and make this document the recognized plan.

- [x] **0.1 Record a green baseline.** Build and run host tests for all five
  packages. Fix nothing yet; if something fails, record it under the task as a
  note and create a `⛔ BLOCKED` marker only if a later task depends on it.

  **Verify:**
  ```sh
  cmake -S lib/microworld -B build/host-core && cmake --build build/host-core && ctest --test-dir build/host-core --output-on-failure
  cmake -S lib/microworld-memory -B build/host-mem && cmake --build build/host-mem && ctest --test-dir build/host-mem --output-on-failure
  cmake -S lib/microworld-object -B build/host-obj && cmake --build build/host-obj && ctest --test-dir build/host-obj --output-on-failure
  cmake -S lib/microworld-engine -B build/host-eng && cmake --build build/host-eng && ctest --test-dir build/host-eng --output-on-failure
  cmake -S lib/microworld-net -B build/host-net && cmake --build build/host-net && ctest --test-dir build/host-net --output-on-failure
  ```
  **Done when:** all five test suites pass (or failures are recorded here as notes).

  **Baseline result — recorded 2026-07-20** (Windows 11; MinGW-w64 UCRT g++
  16.1.0; CMake/CTest 4.0.2; Ninja 1.13.2; Python 3.11.9). CMake's default
  generator on this host is multi-config `Visual Studio 17 2022`, which does not
  satisfy the single-config `ctest --test-dir` form used above, so each configure
  step was run with `-G Ninja -DCMAKE_CXX_COMPILER=g++` appended (plus a harmless,
  ignored `-DCMAKE_C_COMPILER=gcc`) — single-config GCC 16, the strict-warnings
  toolchain already documented in PROGRESS.md. Otherwise the Verify commands were
  run unchanged.

  | Package | Configure + build | Tests | Result |
  | --- | --- | --- | --- |
  | Core (`build/host-core`) | ok | 5/5 passed | ✅ |
  | Memory (`build/host-mem`) | ok | 1/1 passed | ✅ |
  | Object (`build/host-obj`) | ok | 1/1 passed | ✅ |
  | Engine (`build/host-eng`) | **failed** | not reached | 🚫 |
  | Net (`build/host-net`) | **failed** | not reached | 🚫 |

  All five *production* libraries compile; only the two test executables fail to
  build, both inside allocation-counter test support:

  - **Engine + Net — `std::aligned_alloc` is not a member of `std`**
    (`lib/microworld-engine/tests/EngineAllocationCounters.cpp:29`,
    `lib/microworld-net/tests/NetAllocationCounters.cpp:29`). The `AllocateAligned`
    helper has an MSVC branch (`_aligned_malloc`) and a POSIX branch
    (`std::aligned_alloc`); MinGW-w64 UCRT takes the POSIX branch, but its
    libstdc++ does not declare `std::aligned_alloc` (UCRT has no C11
    `aligned_alloc`). Platform gap in test-support code, not production code.
  - **Net — `-Werror=unused-variable` on `ReadDestination`**
    (`lib/microworld-net/tests/NetAllocationTests.cpp:42`): declared but never
    used; GCC 16.1.0 strict warnings reject it.

  Not fixed — task 0.1 is record-only. PROGRESS.md evidence for Engine/Net cites
  GCC/Clang + ASan/UBSan runs (consistent with Linux, where `std::aligned_alloc`
  exists), so this is most likely a Windows/MinGW-only baseline gap.

  ⛔ BLOCKED → **Engine portion RESOLVED 2026-07-20; Net portion still open.**
  The Engine suite was fixed during the merged 1.2+1.3 step by broadening the
  `AllocateAligned` guard in `EngineAllocationCounters.cpp` from `_MSC_VER` to
  `_WIN32` (MinGW then uses `_aligned_malloc`); `build/host-eng` now builds and
  passes here. The **Net** (`build/host-net`) suite still fails to build — same
  `std::aligned_alloc` gap in `NetAllocationCounters.cpp`, plus one
  `-Werror=unused-variable` in `NetAllocationTests.cpp` — and still blocks
  **4.1–4.4** until fixed the same way in Phase 4.

- [x] **0.2 Register this plan in governance docs.** Edit `AGENTS.md` (root)
  and `lib/AGENTS.md`: state that `MICROWORLD_ROADMAP.md` is the improvement
  plan and task tracker, and `lib/microworld/PROGRESS.md` remains the evidence
  record. One or two sentences each, no other changes.

  **Done when:** both files mention this document; no other content changed.

---

### Phase 1 — Consolidation: one Actor model ✅

Goal: remove the duplicate Core actor model so the Engine layer
(`UWorld`/`AActor`/`UActorComponent`) is the **only** World/Actor/Component
API. Core shrinks to shared primitives: `Application.h`, `Lifecycle.h`,
`TickFunction.h`, `Tickable.h`, `Time.h`, `Version.h`.

**Decision record (do not relitigate):** owner chose "retire Core model" on
2026-07-20. Old code stays available in git history.

- [x] **1.1 Inventory dependents of the retired types.** Find every reference
  to `TWorld`, `TActor`, `FActorBase`, `FActorComponent`, `FNetwork` outside
  `lib/microworld/include/MicroWorld/{World,Actor,ActorComponent,Network}.h`
  and their `.cpp` files. Expect hits in: `lib/microworld/tests/`,
  `lib/microworld/examples/HostLifecycle/`, `lib/microworld/benchmarks/`,
  `lib/microworld/tests/consumer/src/`, docs. List the files as notes under
  this task before proceeding.

  **Verify:**
  ```sh
  grep -rn "TWorld\|FActorBase\|FActorComponent\|MicroWorld::FNetwork\|MicroWorld/World.h\|MicroWorld/Actor.h\|MicroWorld/ActorComponent.h\|MicroWorld/Network.h" lib src --include=*.h --include=*.cpp --include=*.md -l
  ```
  **Done when:** the list of affected files is written under this task.

  **Inventory — recorded 2026-07-20.** Verify grep run via `rg` (the repo hook
  blocks `grep`; identical result). The Verify pattern's bare `FActorComponent`
  also matches the Engine types `FActorComponentRegistry` /
  `FActorComponentRegistryBase`, so its raw `-l` output over-reports engine files;
  the list below uses word-boundary matching to isolate genuine retired-type
  references (the same disambiguation task 1.3's own grep applies with `TWorld<` /
  `FActorBase` / `MicroWorld::FNetwork`). Counts are matches per file.

  *Retired type definitions* (the files task 1.2 deletes — not dependents), all
  under `lib/microworld/`: `include/MicroWorld/World.h` (5), `Actor.h` (19),
  `ActorComponent.h` (12), `Network.h` (7); `src/Actor.cpp` (13),
  `ActorComponent.cpp` (7), `Network.cpp` (5). (No `World.h` `.cpp` — `TWorld` is
  header-only.)

  *Genuine dependents — Core package code (migrate or delete in task 1.3):*
  - `lib/microworld/tests/WorldTests.cpp` (40) — delete
  - `lib/microworld/tests/ApplicationNetworkTests.cpp` (18) — delete
  - `lib/microworld/examples/HostLifecycle/Main.cpp` (8) — rewrite on Engine types, move to engine
  - `lib/microworld/benchmarks/DispatchBenchmark.cpp` (15) — port to Engine or delete
  - `lib/microworld/tests/consumer/src/NativeMain.cpp` (2) — retarget to Core primitives
  - `lib/microworld/tests/consumer/src/Esp32Main.cpp` (2) — retarget to Core primitives
  - `lib/microworld/tests/consumer/src/Esp32BenchmarkMain.cpp` (11) — retarget to Core primitives

  *Docs referencing retired types by name (task 1.4 doc sweep; 1.2 covers the
  README public-headers list):* `lib/microworld/README.md` (5), `CHANGELOG.md`
  (1), `AGENTS.md` (2), `docs/UE5ConceptMap.md` (2), `docs/Style.md` (1),
  `docs/ModulePackaging.md` (1), `docs/decisions/0001-modular-runtime.md` (1),
  `benchmarks/Results/Host.md` (4). NB: task 1.4 explicitly names only README,
  UE5ConceptMap, PROGRESS, ModulePackaging — the other four above also match and
  fall under 1.4's "no doc references retired types as current API" bar.
  (`PROGRESS.md` itself does **not** match — it names World/Actor/Component only
  generically.)

  *Engine package — stale doc-comment references only, NOT code dependencies
  (no `#include`, no code use); refresh in task 1.4:*
  - `lib/microworld-engine/include/MicroWorld/Engine/Actor.h:29-30` — comment
    "…use FActorBase and TActor from <MicroWorld/Actor.h> instead."
  - `lib/microworld-engine/include/MicroWorld/Engine/World.h:25` — comment
    "…matching Core's non-managed TWorld dispatch."

  *False positives — NOT retired-type dependents (match only
  `FActorComponentRegistry` / `FWorldActorRegistry`; leave untouched):* engine
  `EngineStorage.h`, `EngineRegistryView.h`, `src/Actor.cpp`, `src/World.cpp`,
  `tests/EngineLifecycleTests.cpp`, `tests/EngineRegistrationTests.cpp`,
  `tests/EngineGarbageCollectionTests.cpp`, and
  `lib/microworld/tests/consumer/src/EngineConsumerProbe.h`.

  *Build wiring* (filenames, not type refs — handled by 1.2/1.3):
  `lib/microworld/CMakeLists.txt` lists the retired sources and the retired test
  files. No genuine retired-type reference exists in the top-level `src/` app.

- [x] **1.2 Delete the retired headers and sources.** Remove
  `lib/microworld/include/MicroWorld/World.h`, `Actor.h`, `ActorComponent.h`,
  `Network.h` and `lib/microworld/src/Actor.cpp`, `ActorComponent.cpp`,
  `Network.cpp`. Update `lib/microworld/CMakeLists.txt` target sources and the
  public-headers list in `lib/microworld/README.md`.

  **Done when:** `cmake --build build/host-core` succeeds with the files gone.

- [x] **1.3 Migrate or delete dependent tests/examples/benchmarks.**
  - `lib/microworld/tests/WorldTests.cpp`, `ApplicationNetworkTests.cpp`:
    delete (their engine-level equivalents live in
    `lib/microworld-engine/tests/`). Keep `TickFunctionTests.cpp`.
  - `lib/microworld/examples/HostLifecycle/`: rewrite the same
    sensor-actor demo using `UWorld`/`AActor`/`UActorComponent` and move it to
    `lib/microworld-engine/examples/HostLifecycle/`.
  - `lib/microworld/benchmarks/DispatchBenchmark.cpp`: port to the Engine
    types or delete if `microworld-engine` benchmarks already cover dispatch.
  - Consumer probes under `lib/microworld/tests/consumer/src/` that used Core
    actors: retarget to Core primitives only (tick/lifecycle), since Engine has
    its own consumer probe.

  **Verify:**
  ```sh
  cmake -S lib/microworld -B build/host-core && cmake --build build/host-core && ctest --test-dir build/host-core --output-on-failure
  cmake -S lib/microworld-engine -B build/host-eng && cmake --build build/host-eng && ctest --test-dir build/host-eng --output-on-failure
  grep -rn "TWorld<\|FActorBase\|MicroWorld::FNetwork" lib/microworld lib/microworld-engine --include=*.h --include=*.cpp ; test $? -eq 1
  ```
  **Done when:** both suites pass and the grep finds nothing.

  **Merge outcome (1.2 + 1.3 done together, 2026-07-20).** Per the owner's
  decision in section 6, 1.2 and 1.3 were completed as one step so the Core build
  goes green once. Done here (GCC 16.1.0 via Ninja):
  - Deleted the 7 retired Core files, `tests/WorldTests.cpp`,
    `tests/ApplicationNetworkTests.cpp`, and `benchmarks/DispatchBenchmark.cpp`
    (dispatch is covered by the Engine suite).
  - `HostLifecycle` example rewritten on `UWorld`/`AActor`/`UActorComponent` and
    moved to `lib/microworld-engine/examples/HostLifecycle/Main.cpp`; wired into
    the engine CMake with the strict no-exceptions/RTTI options. It builds and,
    when run, prints the deterministic trace (component-before-actor begin;
    sensor ticks at 0/100/200 with 50 and 175 correctly skipped; actor-before-
    component end).
  - Consumer probes (`NativeMain`, `Esp32Main`, `Esp32BenchmarkMain`) retargeted
    to Core primitives (`FTickFunction`); the Core profile-map check still passes
    (no Object/Engine/Net markers leak into the Core profile). Follow-up
    (2026-07-20): the three shared their probe body into a new
    `CoreConsumerProbe.h` (`RunCoreConsumerProbe()`), matching the
    Memory/Object/Engine family convention — verified by a host GCC build+run of
    the native probe (exit 0) and strict compile-checks of both ESP32 entry
    points; the Xtensa `pio` build stays Phase 5's gate.
  - Core `CMakeLists.txt` (library sources, tests, retired benchmark/example
    targets) and the README public-headers list updated. Two stale Core-type
    references in engine header doc-comments (`Engine/Actor.h`, `Engine/World.h`)
    removed.
  - **Blocker cleared for the Engine suite:** `EngineAllocationCounters.cpp`
    `AllocateAligned` guard broadened `_MSC_VER` → `_WIN32` so MinGW builds.

  Evidence: `build/host-core` 5/5 pass; `build/host-eng` 1/1 pass (+ example
  builds and runs, exit 0); grep gate `rg "TWorld<|FActorBase|MicroWorld::FNetwork"`
  over `lib/microworld` + `lib/microworld-engine` (.h/.cpp) returns no matches;
  Core class-documentation checker passes (35 files). **Not verified this
  session:** the ESP32 PlatformIO consumer builds (no `pio`/hardware here) —
  those are Phase 5's gate; the retargeted probes are trivial Core-primitive
  `app_main`s. Remaining in Phase 1: **1.4** (documentation sweep), which also
  updates the README/UE5ConceptMap prose still describing the retired model.

- [x] **1.4 Documentation sweep for the retirement.** Update
  `lib/microworld/README.md` (Core = primitives only),
  `lib/microworld/docs/UE5ConceptMap.md` (remove Core actor rows; Engine rows
  are the concept map), `lib/microworld/PROGRESS.md` (note the retirement and
  why), `lib/microworld/docs/ModulePackaging.md` if it names retired headers.
  Run the doc checkers.

  **Verify:**
  ```sh
  python lib/microworld/tools/CheckClassDocumentation.py --root lib/microworld
  python lib/microworld/tools/CheckDependencyBoundaries.py --self-test
  # Then run each checker the same way the existing package CI/docs prescribe
  # (see lib/microworld/tools/AGENTS.md for the exact invocation per package).
  ```
  **Done when:** checkers pass; no doc references retired types as current API.

  **Done 2026-07-20 (documentation sweep).** Updated for the retirement:
  `README.md` (Core = lifecycle/tick primitives; the managed Actor model lives
  in Engine), `docs/UE5ConceptMap.md` (dropped the Core `TWorld`/`TActor`/
  `FActorComponent` rows so the managed Engine rows are the World/Actor/Component
  concept map), `PROGRESS.md` (Core "Done" entry reworded to primitives + a
  Phase 1 completion evidence row), `docs/ModulePackaging.md` and
  `docs/decisions/0001-modular-runtime.md` (the deleted Core `FNetwork` boundary
  reframed as retired, not current). Beyond the four named docs, the 1.1
  inventory's remaining retired-type hits were handled to the same bar:
  `AGENTS.md` (Core description → primitives), `docs/Style.md` (T-prefix example
  `TWorld<4>` → `TStaticVector<uint32_t, 4>`), `CHANGELOG.md` (added a "Removed"
  entry; the 0.1.0 release history left intact), and
  `benchmarks/Results/Host.md` (a "Historical (retired model)" note over the
  retired size/dispatch tables).

  Evidence: `CheckClassDocumentation.py` passes (36 files literal / 27 strict);
  `CheckDependencyBoundaries.py --self-test` and `--package Core=lib/microworld`
  pass; a grep for `TWorld`/`TActor`/`FActorBase`/`FActorComponent`/`FNetwork`
  over `lib/microworld/**/*.md` returns only retired/historical-framed hits — no
  doc presents a retired type as current API. Improvement applied alongside: the
  three Core consumer probes now share `CoreConsumerProbe.h` (host GCC build+run
  of the native probe exits 0; both ESP32 entry points compile clean under strict
  flags; Xtensa `pio` build stays Phase 5's gate).

  ⚠️ **Pre-existing gap flagged (not a Phase 1 blocker; see section 6).** The
  prescribed `CheckFolderAgents.py` run fails on `lib/microworld/docs/diagrams:
  missing AGENTS.md`. That directory (roadmap / C4 diagrams) predates Phase 1 and
  has no working-tree change from this task — the strict folder check was already
  red. Whether a generated-diagram directory needs a local guide is a design
  decision the plan does not cover; recorded as a proposed row in section 6 for
  the owner.

---

### Phase 2 — Runtime Spawn & Destroy ⬜

Goal: a UE5 developer can spawn and destroy actors while the world is playing.
Bounded, deterministic, deferred: structural changes apply only at one barrier
point per frame, never in the middle of dispatch.

**Design (follow exactly):**

- Capacity never grows: a world can hold at most `MaxActors` live + pending
  actors (the existing `FWorldActorRegistry<MaxActors>` capacity).
- `UWorld::SpawnActor(TObjectPtr<AActor>)` — allowed while `Playing`. The actor
  must be `Constructed` (never begun), same store, not owned. It goes into a
  bounded **pending-spawn list**; it is registered and receives
  `DispatchBeginPlay` at the next barrier. Returns `EEngineResult`.
- `UWorld::DestroyActor(TObjectPtr<AActor>)` — allowed while `Playing`. The
  actor goes into a bounded **pending-destroy list**; at the barrier it gets
  `DispatchEndPlay`, is removed from the registry (order of survivors
  preserved), and `MarkPendingDestroy` is called on the store so GC reclaims
  it. Its components are ended with it (reverse order) and marked for destroy.
- The barrier is `UWorld::ApplyDeferred(TimePointMilliseconds Now)`, called by
  the application (or `TEngineHost` in Phase 3) **after** `Advance` each frame.
  Order at the barrier: destroys first, then spawns (frees capacity first).
- Registration before `BeginPlay` keeps working exactly as today.
- Components remain fixed per actor at construction (no runtime component
  add/remove — out of scope).

**Exact API to add to `UWorld` (Engine/World.h):**

```cpp
/** Queues one constructed, same-store, unowned actor for begin at the next barrier. */
EEngineResult SpawnActor(TObjectPtr<AActor> Actor) noexcept;

/** Queues one actor registered with this world for end+release at the next barrier. */
EEngineResult DestroyActor(TObjectPtr<AActor> Actor) noexcept;

/** Applies pending destroys (first) then pending spawns; call once per frame after Advance. */
ERuntimeResult ApplyDeferred(TimePointMilliseconds NowMilliseconds) noexcept;

/** Report pending structural work so tests and budget policy can observe it. */
std::size_t PendingSpawnCount() const noexcept;
std::size_t PendingDestroyCount() const noexcept;
```

**Validation → result mapping (must be exact and tested):**

| Condition | `SpawnActor` | `DestroyActor` |
| --- | --- | --- |
| World not `Playing` | `LifecycleLocked` | `LifecycleLocked` |
| Empty/stale/non-resolvable reference | `InvalidReference` | `InvalidReference` |
| Reference from another store | `CrossStore` | `CrossStore` |
| Actor already registered or already pending-spawn | `Duplicate` | — |
| Actor not registered with this world | — | `InvalidReference` |
| Actor already pending-destroy | — | `Duplicate` |
| Actor owned by another world | `AlreadyOwned` | `InvalidReference` |
| live + pendingSpawn == MaxActors | `CapacityExceeded` | — |
| All checks pass | `Success` (queued) | `Success` (queued) |

- [ ] **2.1 Extend registry storage for pending lists.** In
  `lib/microworld-engine/include/MicroWorld/Engine/EngineStorage.h` and
  `EngineRegistryView.h`: give `FWorldActorRegistry<MaxActors>` two extra
  fixed arrays (`PendingSpawn`, `PendingDestroy`, each `MaxActors` slots with
  counts) exposed through the existing lease type, plus a
  `RemoveAt(index)` on the lease that removes one entry and shifts the tail
  left (stable order). Only `UWorld` may call these (existing friend pattern).

  **Done when:** engine package still builds; new lease operations have doc
  comments; no public API leaks mutable storage.

- [ ] **2.2 Implement SpawnActor/DestroyActor/ApplyDeferred on UWorld.** In
  `Engine/World.h` + `src/World.cpp` per the design above. Validation reuses
  the same checks as `RegisterActor` (duplicate, capacity incl. pending,
  cross-store, invalid reference, already-owned, already-pending). All
  failures transactional. `VisitReferences` must also trace pending-spawn
  actors (they are reachable). While `Constructed`, `SpawnActor` returns
  `LifecycleLocked` (use `RegisterActor` instead); after `EndPlay`, both
  return `LifecycleLocked`.

  **Done when:** compiles clean under the package's strict warnings; every new
  method documented; `AActor::DispatchEndPlay` path also marks the actor's
  registered components `MarkPendingDestroy` after their `EndPlay` ran.

- [ ] **2.3 Tests for spawn/destroy.** New
  `lib/microworld-engine/tests/EngineSpawnDestroyTests.cpp` covering at
  minimum: spawn during play begins at next barrier (not immediately); destroy
  during play ends at barrier with reverse-order component shutdown; capacity
  counts live+pending; duplicate spawn rejected; destroy of never-registered
  actor rejected; destroyed actor's handle goes stale after GC; survivor
  dispatch order preserved after mid-list removal; spawn+destroy of the same
  actor in one frame; all rejection paths leave state unchanged (sentinel
  checks); GC cycle after destroy reclaims actor and components with roots and
  worklist accounted.

  **Verify:**
  ```sh
  cmake -S lib/microworld-engine -B build/host-eng && cmake --build build/host-eng && ctest --test-dir build/host-eng --output-on-failure
  ```
  **Done when:** all listed behaviors have passing cases.

- [ ] **2.4 Ergonomics: `TInlineActor<N>` / `TInlineWorld<N>`.** New header
  `lib/microworld-engine/include/MicroWorld/Engine/InlineTypes.h`. Use the
  base-from-member idiom so the registry storage lives inside the object:

  ```cpp
  namespace Detail {
  template<std::size_t N> struct TActorRegistryHolder { FActorComponentRegistry<N> Registry; };
  }
  template<std::size_t N>
  class TInlineActor : private Detail::TActorRegistryHolder<N>, public AActor
  {
  public:
      explicit TInlineActor(FTickConfiguration Cfg = {}) noexcept
          : AActor(this->Registry.MakeView(), Cfg) {}
  };
  // TInlineWorld<N> mirrors this over FWorldActorRegistry<N> and UWorld.
  ```

  Derived user classes extend `TInlineActor<N>` and register their components
  in their constructor. Add a test that builds a world + actor + component
  using only inline types, and rewrite the Phase 1 HostLifecycle example with
  them. **Note:** each concrete inline instantiation is its own managed type —
  it needs its own `FClassDescriptor` registered (document this in the header).

  **Done when:** example + test compile and pass using inline types only;
  doc comments explain the descriptor requirement.

---

### Phase 3 — Engine composition root & logging ⬜

Goal: one type that wires the whole runtime, and a logging facade. After this
phase a "hello world" app is ~20 lines, and every app shares the same frame
order.

- [ ] **3.1 `MW_LOG` logging facade.** New header
  `lib/microworld/include/MicroWorld/Log.h` (Core owns it; every package may
  use it). Requirements:
  - Levels: `Error`, `Warning`, `Log`, `Verbose`.
  - Compile-time floor: `MW_LOG_MIN_LEVEL` (default `Log`); calls below the
    floor compile to nothing (zero code, zero strings in flash).
  - One process-global sink: `using FLogSink = void(*)(ELogLevel, const char* Category, const char* Message)`,
    set via `SetLogSink(FLogSink)`. Default sink is null → logging disabled.
    No allocation, no formatting inside Core: the macro forwards a
    printf-style format + args to the sink adapter only if you can do it
    without allocation — otherwise keep the sink signature message-only and
    let callers format into a caller-owned buffer. Choose the simplest option
    and document it.
  - Usage: `MW_LOG(Warning, "Net", "peer %u timed out", Index);` or the
    message-only variant.
  - Tests: floor stripping (a below-floor call must not evaluate arguments),
    sink routing, null-sink safety.

  **Verify:** Core package build + new tests pass.
  **Done when:** requirements above met; README of Core documents the macro.

- [ ] **3.2 `TEngineHost` composition template.** New header
  `lib/microworld-engine/include/MicroWorld/Engine/EngineHost.h`:

  ```cpp
  template<
      std::size_t MaxClasses, std::size_t MaxObjects, std::size_t SlotBytes,
      std::size_t SlotAlign, std::size_t MaxRoots, std::size_t MaxActors,
      std::size_t MaxTimers, std::size_t TimerCallbackBytes>
  class TEngineHost final { ... };
  ```

  Owns (as members, in construction order): class registry, object-store
  storage arrays, `FObjectStore`, GC worklist storage, `FGarbageCollector`,
  `FWorldActorRegistry<MaxActors>`, `TTimerManager<MaxTimers, TimerCallbackBytes>`.
  API:
  - `EObjectResult RegisterClass(const FClassDescriptor&)` — forwards to registry;
    engine base descriptors (`UWorld`, `AActor`, `UActorComponent`) registered
    automatically in the constructor.
  - `TObjectPtr<UWorld> CreateWorld()` — constructs the `UWorld` in the store,
    takes one `TStrongObjectPtr<UWorld>` root, stores it.
  - `template<typename T, ...Args> TObjectCreationResult<T> NewObject(Args&&...)`
    — forwards to `FObjectStore::NewObject<T>` (which already exists).
  - `TTimerManager& GetTimerManager()`, `UWorld& GetWorld()`,
    `FObjectStore& GetObjectStore()`.
  - `ERuntimeResult BeginPlay(Now)`, `ERuntimeResult Tick(Now)`,
    `ERuntimeResult EndPlay()` — `Tick` runs the canonical frame order from
    section 4 (net steps become live in Phase 4; leave clearly marked call
    slots until then). GC each tick: call `RequestCollection()` if the
    collector phase is `Idle`, then `Advance(Budget)`; the
    `FGarbageCollectionBudget` is a constructor parameter.
  - No heap use anywhere; `TEngineHost` itself is meant to live in static
    storage or `main`'s stack frame.

- [ ] **3.3 Tests + example for the host.**
  `lib/microworld-engine/tests/EngineHostTests.cpp`: begin/tick/end lifecycle;
  frame order observable via instrumented timer callback vs. actor tick vs. GC
  reclamation (spawn garbage, verify bounded reclamation across ticks);
  monotonic-time rejection; idempotent EndPlay. Rewrite the HostLifecycle
  example (from 2.4) on top of `TEngineHost` — that example is now the
  canonical "hello MicroWorld" and gets linked from the README.

  **Verify:** engine package build + tests; example runs and prints expected
  lifecycle trace.
  **Done when:** example ≤ ~60 lines of user code; tests pass.

---

### Phase 4 — Simple networking with roles ⬜

Goal: the UE5 *concept* of dedicated server / listen server / client, delivered
as simple bounded message passing. **No replication, no RPC, no property sync.**
One driver, one peer table, channels, heartbeats. Everything host-testable via
loopback.

**Design (follow exactly):**

- `FNetAddress` (in `microworld-net`): opaque driver-defined bytes.
  `std::array<std::uint8_t, 12> Bytes; std::uint8_t Size;` with `operator==`.
  UDP uses IPv4+port (6 bytes), LoRa uses a 1–2 byte node id, loopback uses 1
  byte. Drivers document their encoding.
- `INetDriver` v2 (breaking change):
  ```cpp
  virtual ENetResult TrySend(const FNetAddress& To, TSpan<const std::uint8_t> Packet) noexcept = 0;
  virtual ENetResult TryReceive(FNetAddress& OutFrom, TSpan<std::uint8_t> Dest, FNetReceiveResult& Out) noexcept = 0;
  virtual std::size_t MaxPacketBytes() const noexcept = 0;
  ```
  Same transactional semantics as today. Update `FNetManager` (queue entries
  gain a destination address) and `FHostLoopback` (multi-endpoint: N bounded
  mailboxes keyed by 1-byte address, so client/server tests run in-process).
- Message header (on the wire, after any driver framing):
  `[u8 Channel][u8 Flags][u16 PayloadBytes]` little-endian via
  `FByteWriter`/`FByteReader`. Channel 0 is reserved for session control;
  channels 1–255 are application-defined. Flags reserved, must be 0.
- Session control messages (channel 0, first payload byte = type):
  `Hello=1` (client→server), `Welcome=2` (server→client, carries assigned peer
  index), `Heartbeat=3` (both directions on a configured interval), `Bye=4`.
- `ENetMode { Standalone, Client, ListenServer, DedicatedServer }`.
- `TNetHost<MaxPeers, MaxPacketBytes>` (new header `Net/NetHost.h`):
  - `Configure(ENetMode, INetDriver&, FNetHostConfig)` where config carries
    heartbeat interval ms, peer timeout ms, server address (client mode).
  - `Start(Now)`, `Stop()`.
  - `PumpReceive(Now)`: drain driver (bounded per call: at most `MaxPeers + 4`
    receives), handle channel 0 internally (peer admission — bounded table
    with generation-checked `FPeerId{u8 Index, u8 Generation}` —, heartbeat
    bookkeeping, timeout eviction), dispatch channels ≥1 to the registered
    handler: one `TMulticastDelegate<void(FPeerId, std::uint8_t Channel,
    TSpan<const std::uint8_t>), MaxHandlers, InlineBytes>` (reuse Memory's
    multicast delegate; pick small fixed capacities and document them).
  - `SendTo(FPeerId, Channel, payload)`, `Broadcast(Channel, payload)` —
    build header + payload into the outbound FIFO (`FNetManager`).
  - `PumpSend(Now)`: emit due heartbeats, then drain the FIFO via
    `AdvanceSend` (bounded per call).
  - Role semantics: `DedicatedServer`/`ListenServer` accept `Hello` up to
    `MaxPeers`; `Client` holds exactly one peer (the server) and sends `Hello`
    on `Start`; `Standalone` runs no driver traffic and reports `Unavailable`
    on send. `ListenServer` additionally owns a **local peer**: messages sent
    to it are dispatched directly to the local handler without the driver
    (this is what makes it a listen server rather than a naming gimmick).
- Peers who miss heartbeats for the timeout window are evicted; their
  `FPeerId` generation bumps so stale ids fail safely.

**Wire format (exact byte layout, little-endian):**

| Offset | Size | Field | Rule |
| --- | --- | --- | --- |
| 0 | 1 | `Channel` | 0 = session control, 1–255 = application |
| 1 | 1 | `Flags` | Must be 0; receiver drops packet if nonzero |
| 2 | 2 | `PayloadBytes` | u16 LE; receiver drops packet if it disagrees with actual packet size |
| 4 | N | `Payload` | N == `PayloadBytes` |

**Control payloads (channel 0, first payload byte = message type):**

| Type | Name | Payload after type byte | Direction |
| --- | --- | --- | --- |
| 1 | `Hello` | `[u8 ProtocolVersion]` | client → server |
| 2 | `Welcome` | `[u8 ProtocolVersion][u8 PeerIndex][u8 PeerGeneration]` | server → client |
| 3 | `Heartbeat` | (empty) | both |
| 4 | `Bye` | (empty) | both |

Version mismatch: server ignores the `Hello` (no reply) and logs via `MW_LOG`.
Unknown control type / malformed control payload: drop, log, take no action.

**Core structs (exact fields):**

```cpp
struct FNetHostConfig
{
    DurationMilliseconds HeartbeatIntervalMilliseconds{1000};
    DurationMilliseconds PeerTimeoutMilliseconds{5000};
    FNetAddress ServerAddress{};      // Client mode only; ignored otherwise.
    std::uint8_t ProtocolVersion{1};
};

struct FPeerId { std::uint8_t Index{0xFF}; std::uint8_t Generation{0}; };

struct FNetPeerSlot                    // internal to TNetHost
{
    FNetAddress Address{};
    TimePointMilliseconds LastReceiveMilliseconds{0};
    TimePointMilliseconds LastSendMilliseconds{0};   // paces heartbeats
    std::uint8_t Generation{0};
    bool bActive{false};
};
```

**Peer state machines (implement exactly; test every transition):**

Server slot: `Free` → (`Hello` received, free slot, version ok) → `Active`
(send `Welcome`) → (`Bye` received OR `Now - LastReceive > PeerTimeout`) →
`Free`, `Generation += 1`. A repeated `Hello` from an already-active address
re-sends `Welcome` (idempotent), does not allocate a second slot.

Client: `Idle` → `Start()` → `Connecting` (send `Hello`; re-send each
heartbeat interval) → (`Welcome` received) → `Connected` (heartbeats flow) →
(timeout) → `Connecting` again (fresh `Hello`). Application can observe state
via `ENetHostState GetState() const noexcept`.

- [ ] **4.1 `FNetAddress` + `INetDriver` v2 + migrate `FNetManager` and
  `FHostLoopback`.** Update all existing net tests to the addressed API.
  Loopback becomes multi-endpoint as described.

  **Verify:** net package build + all tests pass.
  **Done when:** no unaddressed send/receive API remains; loopback supports at
  least 4 endpoints in tests.

- [ ] **4.2 Message framing + session control.** New `Net/NetProtocol.h` with
  header/read/write helpers and control-message encode/decode built on
  ByteWriter/Reader. Pure functions, fully unit-tested (round trip, truncation,
  bad flags, unknown control type).

  **Done when:** framing tests pass; no allocation; all failures transactional.

- [ ] **4.3 `TNetHost` with roles.** Implement per the design. Tests (over
  multi-endpoint loopback): server admits client (Hello→Welcome), peer table
  capacity rejection, heartbeat keeps peer alive, missed heartbeats evict,
  stale `FPeerId` rejected after eviction, client reconnect gets new
  generation, broadcast reaches all peers, listen-server local peer receives
  without driver traffic, dedicated server has no local dispatch, `Standalone`
  sends report `Unavailable`, bounded pumps (a flooded driver cannot starve
  the frame).

  **Verify:** net package build + tests.
  **Done when:** every listed behavior has a passing case.

- [ ] **4.4 Wire `TNetHost` into `TEngineHost`.** Optional slot: a
  `TEngineHost` constructor overload accepts a `TNetHost&` (caller-owned) and
  the canonical frame order from section 4 becomes fully live (receive pump
  first, send pump last). Add one engine-level test: two `TEngineHost`
  instances over loopback exchange a message that spawns an actor on the
  server world — the "concept proof" that net + engine compose.

  **Done when:** test passes; frame order documented in `EngineHost.h`.

---

### Phase 5 — Platform adapters (ESP32 + host) ⬜

Goal: MicroWorld runs on the real board. New packages; portable packages stay
platform-free (dependency checker must keep passing).

- [ ] **5.1 `microworld-platform-host` package.** `lib/microworld-platform-host/`
  with: `FHostTimeSource` (steady_clock → ms since start) and
  `FHostUdpDriver` implementing `INetDriver` v2 over BSD/WinSock UDP
  (non-blocking sockets; `FNetAddress` = IPv4 + port). CMake + tests (two
  drivers on localhost exchange packets; `Unavailable` on empty;
  `Invalid`/`Full` mapping documented and tested where the OS allows).

  **Done when:** a host demo sends `TNetHost` traffic over real UDP localhost.

- [ ] **5.2 `microworld-platform-esp32` package.** `lib/microworld-platform-esp32/`
  (ESP-IDF component style, PlatformIO-compatible like the existing consumer
  probes): `FEsp32TimeSource` (`esp_timer_get_time()/1000`), `FEsp32UdpDriver`
  (lwIP non-blocking UDP, same address encoding as host driver), a log sink
  adapter to `ESP_LOG`, and an `app_main` glue example running `TEngineHost`
  at a fixed cadence via `vTaskDelay`. **Compile-only evidence** (like existing
  `Esp32S3N16R8.md` records); no upload without explicit authorization.

  **Verify:**
  ```sh
  pio run -d lib/microworld/tests/consumer -e esp32-s3   # existing probes still build
  # plus the new esp32 example environment added by this task
  ```
  **Done when:** ESP32 image builds; RAM/flash recorded in the package's
  `benchmarks/Results/Esp32S3N16R8.md`.

- [ ] **5.3 E32 LoRa UART driver.** `FEsp32E32LoraDriver` in the esp32 package:
  UART framing `[u8 0xA5][u8 SrcNodeId][u16 Len][payload][u16 CRC16-CCITT]`,
  bounded RX state machine (resync on bad magic/CRC), `FNetAddress` = 1-byte
  node id, respects E32 payload limits via `MaxPacketBytes()`. Host-side unit
  tests for the framing state machine (feed byte streams incl. corruption,
  truncation, resync) — the state machine must be a portable class in
  `microworld-net` (`Net/FrameCodec.h`) so it is testable off-target; only the
  UART glue lives in the esp32 package.

  **Done when:** FrameCodec host tests pass; esp32 package compiles with the
  driver; no radio transmission performed.

---

### Phase 6 — Examples, measurement & release hardening ⬜

- [ ] **6.1 Two-node demo.** `examples/` app: dedicated server on host (UDP),
  ESP32 or second host process as client; client button/keyboard event spawns
  an actor server-side, server broadcasts state at a heartbeat cadence.
  This is the acceptance demo for "UE5 dev can build a small networked thing".

- [ ] **6.2 Measure runtime margins.** Every PROGRESS.md row says "target
  runtime margins unmeasured". Measure on ESP32-S3 (requires explicit human
  authorization to flash): tick duration for a representative world (e.g. 8
  actors / 16 components / 8 timers / GC budget slice), max GC pause per
  budget unit, net pump cost, RAM/flash of the full image. Record in the
  package `benchmarks/Results/` files and PROGRESS.md.

- [ ] **6.3 Documentation release sweep.** Update: `lib/microworld/README.md`,
  `lib/microworld-engine/README.md`, `lib/microworld-net/README.md`,
  `UE5ConceptMap.md` (add Spawn/Destroy, TEngineHost, ENetMode/TNetHost rows),
  `Porting.md` (how to write a platform adapter: time source + net driver +
  log sink = one page), `PROGRESS.md` final status, `CHANGELOG.md`, bump
  version to `0.2.0` in `Version.h`. Run all checker scripts.

- [ ] **6.4 Final acceptance.** All packages build + test on host; ESP32
  images compile; dependency/doc checkers pass; the two-node demo runs; this
  document's tracker is fully ✅ except this line, which flips last.

---

## 6. Design decisions record

| Date | Decision | Chosen by |
| --- | --- | --- |
| 2026-07-20 | Retire Core actor model; Engine layer is the only World/Actor/Component API | Owner |
| 2026-07-20 | Runtime SpawnActor/DestroyActor is a must-have (deferred-barrier design) | Owner |
| 2026-07-20 | Networking = simple channel messages with roles (no replication/RPC); transports: UDP first (host-testable), E32 LoRa second, ESP-NOW deferred | Owner |
| 2026-07-20 | Components fixed at actor construction; no runtime component add/remove | Plan default |
| 2026-07-20 | GC stays optional-but-default via TEngineHost budget; store+GC design kept as-is | Plan default |
| 2026-07-20 | **Merge tasks 1.2 + 1.3 into one combined step** (delete the retired headers/sources *and* migrate/delete every retired-type dependent together), because 1.2's Done-when (green `build/host-core`) cannot hold while 1.3's dependents still `#include` the deleted headers. The Core build goes green once, at the end of the combined step. | Owner |
| 2026-07-20 | **PROPOSED (awaiting owner):** `lib/microworld/docs/diagrams` has no `AGENTS.md`, so the prescribed `CheckFolderAgents.py` strict run fails. Pre-existing (predates Phase 1), unrelated to the retirement. Options: (a) add a short `docs/diagrams/AGENTS.md`, (b) add `--exclude diagrams` to the prescribed invocation, or (c) accept it as a generated-assets directory needing no guide (per `tools/AGENTS.md`, the folder check is "not a policy requiring every future package subdirectory to add a local guide"). | PROPOSED |

Add a row here whenever a task forces a design choice not covered by this plan.

---

## 7. Progress tracker

**Update this table and the task checkboxes together.** A phase is 🟨 once any
of its tasks starts, ✅ only when all its tasks are `[x]`.

| Phase | Title | Tasks | Status |
| --- | --- | --- | --- |
| 0 | Baseline & governance | 0.1–0.2 | ✅ |
| 1 | Consolidation: one Actor model | 1.1–1.4 | ✅ |
| 2 | Runtime Spawn & Destroy | 2.1–2.4 | ⬜ |
| 3 | Composition root & logging | 3.1–3.3 | ⬜ |
| 4 | Networking with roles | 4.1–4.4 | ⬜ |
| 5 | Platform adapters | 5.1–5.3 | ⬜ |
| 6 | Examples, measurement, release | 6.1–6.4 | ⬜ |

**Definition of "production ready" (exit criteria):** Phase 6 task 6.4 checked;
version 0.2.0 tagged; a UE5 developer can, following only the READMEs, build a
networked ESP32 application with worlds, actors, components, timers, spawn/
destroy, and client/server messaging — without reading MicroWorld internals.

---

## 8. Appendix A — UE5 → MicroWorld glossary

For workers who know UE5: the mapping and the deliberate differences.

| UE5 concept | MicroWorld equivalent | Deliberate difference |
| --- | --- | --- |
| `UObject` | `UObject` (microworld-object) | No reflection; explicit `VisitReferences` instead of UPROPERTY tracing |
| `UClass` / `StaticClass()` | `FClassDescriptor` / `StaticClassDescriptor()` | Registered explicitly into `TClassRegistry`; no auto-generation |
| `NewObject<T>()` | `FObjectStore::NewObject<T>()` | Fixed equal-size slots; can fail with `CapacityExceeded` |
| `TObjectPtr` | `TObjectPtr` | Resolves {index, generation} on every access; never caches an address |
| `TWeakObjectPtr` | `TWeakObjectPtr` | Same idea; expires on reclaim |
| `AddToRoot()` / `FGCObject` | `TStrongObjectPtr` | Explicit fixed-capacity root table |
| Garbage collector | `FGarbageCollector` | Incremental, caller-budgeted per tick; never runs behind your back |
| `UWorld` | `UWorld` | Fixed actor capacity chosen at compile time |
| `AActor` | `AActor` | Components fixed at construction; no attachment hierarchy/transform |
| `UActorComponent` | `UActorComponent` | No scene components, no transforms |
| `SpawnActor` / `Destroy()` | `UWorld::SpawnActor` / `DestroyActor` (Phase 2) | Deferred to a per-frame barrier; bounded by world capacity |
| `Tick(DeltaSeconds)` | `Tick(const FTickContext&)` | Milliseconds, caller-supplied clock, per-object intervals, no tick groups |
| `FTimerManager` | `TTimerManager<N, Bytes>` | Fixed capacity; OneShot/Looping only; no catch-up bursts |
| Dynamic delegates / events | `TDelegate` / `TMulticastDelegate` | Fixed binding capacity, inline storage, no heap |
| `UGameInstance` + engine loop | `TEngineHost` (Phase 3) | You call `Tick(now)`; there is no hidden main loop |
| `ENetMode` | `ENetMode` (Phase 4) | Same four roles, same meaning |
| `UNetDriver` / NetConnection | `INetDriver` + `TNetHost` peers (Phase 4) | One driver, bounded peer table, no connection object graph |
| Replication / RPC | **none — by design** | Channel-based messages via `SendTo`/`Broadcast`; you serialize with ByteWriter |
| `FMemory` / allocators | `IMemoryResource` / `TFixedArena` | No global allocator; resources are passed explicitly |
| `TArray` (fixed) / `TArrayView` | `TStaticVector` / `TSpan` | No growth, no heap |

## 9. Appendix B — Common mistakes (read before writing code)

Every one of these has either already been prevented by the current design or
will break the ground rules. Check your diff against this list before claiming
a task done.

1. **Never cache a resolved `UObject*` across a frame, barrier, or GC call.**
   Store `TObjectPtr`/`TWeakObjectPtr`/`FObjectHandle` and re-resolve.
   A raw pointer is valid only within the current call scope.
2. **No `std::function`, `std::vector`, `std::string`, `std::map`, `new`,
   `malloc`.** Use `TDelegate` (inline storage), `TStaticVector`,
   `std::array`, fixed char buffers, `IMemoryResource`.
3. **Member-init order trap:** a base class is constructed before members, so
   a member registry cannot be passed to a base constructor. That is exactly
   why `TInlineActor` uses the base-from-member idiom (private storage base
   before `AActor`). Do not "simplify" it into a member.
4. **Check every result enum.** No `(void)Result;`. Tests must assert the
   exact enum value, not just "not Success".
5. **Transactional failure means: validate everything, then mutate.** If a
   function mutates then discovers failure, that is a bug even if tests pass.
   Use sentinel-value checks in tests to prove untouched state.
6. **No platform headers in portable packages.** `esp_*`, `freertos/*`,
   `lwip/*`, `sys/socket.h` may appear only under `lib/microworld-platform-*`.
   The dependency checker enforces direction, not includes — police this
   manually in review.
7. **Time is caller-supplied u64 milliseconds.** Never call
   `std::chrono::steady_clock`, `esp_timer_get_time`, or any clock inside
   portable packages — that includes tests' production code paths.
8. **Each concrete managed type needs its own `FClassDescriptor`** with a
   unique `FTypeId`, correct parent pointer, and exact size/alignment. This
   includes every `TInlineActor<N>` instantiation you derive from. Forgetting
   the parent chain breaks `IsChildOf` and store validation.
9. **Registration order is contract.** Dispatch, shutdown (reverse), and trace
   order all derive from it. Never replace ordered arrays with anything
   unordered.
10. **`noexcept` everywhere on the public API.** The packages build with
    exceptions disabled; a missing `noexcept` is an API-consistency bug.
11. **Doc comments are mandatory** on every function declaration and
    persistent member — `CheckClassDocumentation.py` fails otherwise. Write
    intent/invariant/ownership, not a restatement of the signature.
12. **Don't widen scope.** If a task tempts you to add a feature from section
    3.4 (out of scope), stop and add a note instead. Smallest usable milestone
    wins.

