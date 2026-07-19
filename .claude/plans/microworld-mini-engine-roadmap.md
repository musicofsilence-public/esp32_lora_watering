# MicroWorld Post-0.1 Execution Roadmap

| Field | Value |
| --- | --- |
| Created | 2026-07-19 |
| Status | Live state: [MicroWorld progress](../../lib/microworld/PROGRESS.md) |
| Scope | Post-0.1 portable modules, evidence gates, and release decisions |
| Authority | [Mini-engine concept](../concepts/microworld-mini-engine-roadmap.md) |
| Archive | `git show cf5d964:.claude/plans/microworld-mini-engine-roadmap.md` |

> **Archive note.** The prior detailed design, C++ sketches, and exhaustive task
> breakdown remain recoverable at the archive anchor above. This document is the
> concise executable roadmap; it intentionally does not duplicate the concept or
> report live completion.
> Historical detail is retained for auditability, not as an instruction to start deferred work.

## Purpose and document authority

MicroWorld 0.1 is the released deterministic lifecycle/tick kernel. This
roadmap sequences its measured post-0.1 evolution without changing that release
contract by implication.

| Source | Owns |
| --- | --- |
| [Concept](../concepts/microworld-mini-engine-roadmap.md) | Durable architecture decisions, boundaries, and open questions |
| This roadmap | Ordered work, gates, and required decisions |
| [PROGRESS](../../lib/microworld/PROGRESS.md) | Current implementation, gate state, blockers, and evidence links |
| ADRs | Accepted decisions and their reconsideration triggers |
| Public headers and tests | Implemented behavioral contract |
| Benchmark and evidence documents | Measured build, resource, and runtime facts |

When sources disagree, released behavior comes from headers/tests and measured
claims come only from evidence records. A roadmap or ADR never upgrades a
candidate to released behavior.

## Product boundary and non-goals

The product is a layered, UE-shaped, MCU-native runtime. It borrows concepts,
vocabulary, lifecycle, and ownership patterns familiar to UE5 C++ developers;
it is not source-, ABI-, or desktop-engine-compatible with UE5.

Out of scope until a separately approved concept proves a need:

- UE source/ABI compatibility, generated reflection, Blueprint, editor tools,
  rendering, physics, audio, navigation, animation, asset systems, GAS, ECS,
  and a general task graph;
- unrestricted heap use, moving or conservative GC, ISR/background collection,
  transparent thread safety, or guaranteed real-time scheduling;
- full replication, RPCs, prediction, rollback, relevancy, or travel;
- a speculative universal peripheral HAL;
- E32 framing, valve policy, authentication keys, hardware pin selection, or
  any remote-controller safety behavior.

The remote-controller tutorial remains a pinned consumer. It must not redesign
MicroWorld during a lesson or move fail-closed product policy into the runtime.

## Module dependency model

```text
Core <- Memory <- Object <- Engine
Core <- Serialization <- Net
Memory <----------------- Net

Engine + Net <- optional Engine-Net bridge
portable modules <- host / ESP-IDF / STM32 / Pico port packages
```

Dependencies point inward. Core remains independently useful. Memory supplies
non-managed ownership. Object supplies managed identity and collection. Engine
adds managed World/Actor/Component behavior. Net is an overlay usable with Core
or Managed composition and does not depend on Engine. The optional bridge owns
their scheduling relationship. Ports adapt SDK services and never pull vendor
headers into portable modules.

Module boundaries are release boundaries as well as source boundaries. A new
module may depend only on its stated inward contracts, must expose a narrow
public API, and must provide an independently selectable consumer probe. Core
does not acquire a feature macro that admits candidate sources conditionally;
applications compose adjacent packages instead. A port package may translate
clock, logging, memory diagnostics, entropy, critical-section, or byte-stream
services, but it does not select application policy or change engine ownership.

## Invariants

- Portable modules require C++17 and must not rely on exceptions or RTTI.
- Work, queues, object stores, delegates, and steady-state storage are bounded.
  Failure is typed and observable; no fallback allocation is silent.
- Caller-supplied monotonic time is the only scheduling time source.
- Allocation through a selected resource is explicit. No module silently grows
  the heap or performs an emergency collection.
- Managed GC is optional, fixed-capacity by default, non-moving, explicit-root,
  metadata-guided, iterative, and incrementally operation-budgeted.
- GC never scans arbitrary stacks, runs in an ISR, reads a hidden clock, starts
  a background thread, or owns deterministic hardware/safety services.
- Hardware drivers, ISR state, watchdog paths, transport implementations,
  credentials, and fail-closed policy use deterministic ownership outside GC.
- Serialization names byte order, widths, lengths, versions, and failures.
  No API sends ABI-dependent objects, raw pointers, or object handles as wire
  identities.
- Cryptographic primitive, board, transport, target budget, and port claim each
  require an explicit decision and evidence; compile-only success is never a
  runtime, timing, stack, heap, radio, or hardware claim.
- Borrowed UE names require explicit semantic differences and tests. `U`/`A`
  names are reserved for real managed contracts, not cosmetic renaming.

## Profile composition

| Composition | Included modules | Intent |
| --- | --- | --- |
| Core | Core | Deterministic lifecycle, tick, bounded registration, diagnostics |
| Memory | Core + Memory | Explicit resources, bounded utilities, non-managed ownership |
| Object | Core + Memory + Object | Managed identity, handles, roots, and bounded collection foundation |
| Managed | Core + Memory + Object + Engine | Managed World/Actor/Component runtime |
| Core + Net | Core + Memory + Serialization + Net | Networked deterministic application without GC |
| Managed + Net | Managed + Serialization + Net (+ optional bridge) | Managed application with independent Net overlay |

Profiles are composition choices, not a preprocessor monolith. Each selected
module must carry explicit flash, RAM, stack, allocation, and bounded-work
evidence before promotion.

## Gate definitions

Each gate has three distinct states: technical evidence, owner decision, and
promotion eligibility. Current state belongs only in PROGRESS.

| Gate | Technical exit evidence | Required owner decision | Promotion meaning |
| --- | --- | --- | --- |
| A Baseline | Exact source, host tests/maps/benchmarks, and ESP compile probes reproduce | Accept baseline evidence | v0.1 baseline may anchor later comparisons |
| B Modules | Core-only maps exclude later modules; CMake/PlatformIO packaging checks pass | Accept module/package boundary | Core boundary may be used by candidates |
| C Memory | Ownership/OOM/allocation tests, sanitizers where available, and target evidence are recorded | Accept retained design and target margin | Memory may leave experimental status only with accepted margins |
| D Objects | Handle/root/cycle/destruction/budget tests and sanitizers pass; object evidence is recorded | Accept managed-object contract and margins | Managed object API may be promoted only after approval |
| E Engine | Mutation barriers, lifecycle order, frame-failure rules, timers, and managed example pass | Accept engine failure semantics and resource cost | Dynamic managed runtime may be released |
| F Net | Archive vectors, malformed input, session, budget, backpressure, fuzz, and loopback evidence pass | Accept transport/security scope and any replication decision | Net candidate may precede a real target driver |
| G Ports | Exact board/toolchain compile and authorized runtime evidence are recorded | Accept each port's support level and margins | Named port status may change from experimental |
| H Release | Documentation, migration, packages, profiles, two applications, and two MCU families pass | Approve release scope/version | 1.0 candidate may become stable |

## Ordered implementation phases

### Foundation phases A–D

The following scopes define the foundation sequence. Their live evidence and
decisions are linked from PROGRESS rather than duplicated here.

| Gate | Scope definition | Required evidence location |
| --- | --- | --- |
| A | Reproducible v0.1 baseline, host behavior suite, compile probes, maps, and benchmarks | Core benchmark results and PROGRESS |
| B | `MicroWorld::Core`, adjacent package strategy, dependency boundary and profile-map checks | Module packaging evidence and PROGRESS |
| C | Explicit memory resources, unique/shared ownership, bounded containers, delegates, and resource comparison | Memory ADR/evidence and PROGRESS |
| D | Object handles, descriptor registry, roots, object store, and bounded incremental collector | [Host Object evidence](../../lib/microworld-object/benchmarks/Results/Host.md), [ESP32 compile evidence](../../lib/microworld-object/benchmarks/Results/Esp32S3N16R8.md), and PROGRESS |

### Gate E — managed engine runtime

Do not start production Engine files until Gate D has the owner decision
required by its definition.

| Old ID | Ordered task | Outcome |
| --- | --- | --- |
| 35A | Define engine frame-failure semantics | ADR covers phase result, cleanup, retry, and terminal paths |
| 36 | Declare subsystem phases | Externally owned deterministic phase contract |
| 37 | Declare managed Actor | Weak World / strong Component relationship is explicit |
| 38 | Declare managed Component | Weak owner and lifecycle contract are explicit |
| 39–40 | Declare and implement managed World dispatch | Bounded spawn/destroy queues and mutation barriers |
| 41 | Declare bounded timers | Generation-safe bounded callbacks without catch-up bursts |
| 42–43 | Declare and implement engine loop | Fixed phase order, errors, and GC scheduling |
| 44–45 | Test managed runtime and timers | Behavior tests for order, failure, mutation, cadence, and capacity |
| 46 | Add host Managed example | Public trace demonstrates documented lifecycle |

Gate E verification includes no mutation during traversal, no child/parent
ownership cycle, explicit failure propagation, no hidden allocation, and an
accepted resource record.

### Gate F — serialization and networking

| Old ID | Task | Exit condition |
| --- | --- | --- |
| 47–49 | Declare, implement, and test byte archives | Endian/bounds/version golden vectors; no raw-object API |
| 50 | Declare Net types and driver | Transport-only non-blocking contract |
| 51–52 | Declare and implement NetManager | Validation, sessions, queues, budgets, and counters remain bounded |
| 52A | Add optional Engine-Net adapter | Separate integration target; Net remains Engine-free |
| 53 | Add host loopback driver | Deterministic non-blocking reference transport |
| 54–57 | Tests, fuzz target, benchmark, and loopback example | Invalid input cannot dispatch; budget/backpressure evidence recorded |
| 58–59 | Prototype descriptors and decide replication | Keep preview separate or remove based on real-consumer evidence |

No real driver, protocol authentication claim, or replication promotion occurs
without the owner decisions named by Gate F.

### Gate G — platform contracts and ports

| Old ID | Task | Exit condition |
| --- | --- | --- |
| 60 | Declare narrow platform services | Contract is justified by two ports or an accepted module |
| 61–62 | Add host and ESP-IDF port packages | Shared contracts and selected profiles pass without upload claims |
| 63–64 | Select and add STM32 target | Exact board/SDK/toolchain and evidence are recorded |
| 65–66 | Select and add Pico target | Exact board/SDK/toolchain and evidence are recorded |
| 67 | Add shared port-contract tests | Every maintained port runs one behavioral suite |

### Gate H — release hardening

| Old ID | Task | Exit condition |
| --- | --- | --- |
| 68–71 | Ownership, architecture, networking, and migration guides | Released headers, tests, and semantic differences agree |
| 72 | Add starter templates | New sample needs no framework patch |
| 73 | Run cross-profile verification | Every applicable gate is reproducible |
| 74 | Update release metadata | Version, manifests, changelog, maturity, and limitations agree |

## Cross-cutting verification

| Area | Required check |
| --- | --- |
| Boundaries | No vendor SDK/product-policy dependencies in portable modules; maps enforce selected profile composition |
| Language | C++17, strict warnings, no-exceptions/no-RTTI consumer probes |
| Ownership | Positive, negative, capacity, stale-handle, OOM, destruction, and re-entry behavior tests |
| Collection | Root/cycle/weak/reclamation/budget tests; sanitizer evidence where available |
| Engine | Lifecycle, mutation barriers, phase failures, timer cadence, and managed example trace |
| Serialization/Net | Golden vectors, truncation and malformed inputs, fuzz corpus, session/sequence rejection, queues and backpressure |
| Resources | Same target/profile comparisons for map, static RAM, arena, stack, allocations, and bounded work |
| Ports | Compile evidence and authorized runtime evidence recorded separately |
| Documentation | Header contracts, semantic map, ownership/profile/porting/migration guides, and scoped AGENTS coverage |

## Resource and evidence rules

- Resource limits are unresolved until a named application/owner accepts the
  remaining margin. Do not convert host sizes or whole-image maps into target
  budgets.
- A test or compile passes only the stated behavior or compile claim. It does
  not establish electrical behavior, radio performance, stack margin, latency,
  heap delta, or physical port support.
- Measure a representative fixed workload before retaining an optimization.
  Unexplained same-target/profile regression above the recorded 10% envelope
  blocks promotion.
- Linker maps prove composition, not runtime cost. Hardware runtime evidence
  requires explicit authorization and records source, board, toolchain, flags,
  workload, measurement method, and results.

### Evidence record minimums

| Evidence type | Record before using it as a gate input |
| --- | --- |
| Host behavior | Exact source revision, compiler, configuration, named tests, and result |
| Compile probe | Consumer, package/version, board environment, toolchain, flags, and map path |
| Sanitizer run | Sanitizer/toolchain, selected suite, result, and any environment limitation |
| Benchmark | Workload, warm-up, repetitions, statistics, allocation counter, and comparison baseline |
| Target runtime | Board, power/clock configuration, firmware revision, authorization, method, raw observation, and limitations |
| Owner decision | Alternatives, accepted trade-off, scope affected, and remaining caveat |

### Required decision records

| Decision | Needed before | Minimum question to resolve |
| --- | --- | --- |
| Managed resource margin | Object/Engine promotion | Which capacities and remaining target margin are accepted? |
| Engine frame failure | Engine public release | Which phase failures retry, clean up, stop, or require application reset? |
| Wire format | First Net packet | What are framing, versions, identities, lengths, and compatibility rules? |
| Authentication | Security claim or real transport | What threat is addressed, how are keys provisioned, and how is replay handled? |
| Port selection | STM32/Pico support claim | Which exact board, SDK, compiler, build flags, and profile budget apply? |
| Replication scope | Any promotion beyond preview | What authority, identity, compatibility, and bandwidth evidence justify it? |

### Expected artifacts by phase

| Gate | Contracts and code | Tests and measurements | Documentation |
| --- | --- | --- | --- |
| A–B | Core target/package boundaries | Host/consumer probes, maps, baseline workloads | Packaging and baseline evidence |
| C | Memory resources, ownership, containers, delegates | OOM, lifetime, allocation, sanitizer, and target-margin evidence | Pointer-foundation ADR and budget record |
| D | Handles, descriptors, store, roots, collector | Cycles, stale handles, budgets, re-entry, sanitizers | Managed-memory ADR and object evidence |
| E | Engine, World, Actors, Components, timers | Mutation, lifecycle, failure, cadence, managed trace | Engine failure ADR and ownership guide |
| F | Archives, driver, manager, optional bridge | Golden vectors, fuzz, loopback, queue/backpressure work | Networking and replication-scope decisions |
| G | Platform contracts and port packages | Contract suite, compile probe, authorized runtime record | Porting matrix and target decisions |
| H | Stable package metadata and templates | Candidate suite across applications and MCU families | Migration, semantic map, limitations, release notes |

## Key risks and controls

| Risk | Control |
| --- | --- |
| Familiar names imply UE behavior | Semantic mapping, concise contracts, and behavior tests |
| GC/shared ownership introduces memory defects | Smallest semantics, fixed storage, adversarial tests, and separate gates |
| Managed and deterministic lifetimes blur | Distinct types; hardware and safety never GC-owned |
| Fixed slots waste RAM | Measure real object distributions before adding allocator complexity |
| Bounded GC disrupts updates | Operation budgets, no hidden clocks, and maximum-graph tests |
| Deferred mutation surprises callers | Document barriers and make queue failures observable |
| Generic HAL grows speculative | Require two ports or accepted module need |
| Net conflates transport, reliability, and security | Driver/manager/policy boundaries and no unearned claims |
| Release scope outruns evidence | Stop at each gate pending owner decision |

### Boundary review questions

Before a gate is promoted, reviewers must be able to answer these questions from
the headers, tests, and evidence without relying on implementation folklore:

- Does one concrete owner control each memory resource, physical peripheral,
  transport adapter, and lifecycle boundary?
- Can the selected profile omit every unselected module at link time?
- Does every queue, collection slice, archive, and parser have a stated maximum
  unit of work and an explicit full/invalid result?
- Is a managed reference visibly different from unique ownership, shared
  ownership, weak observation, and a short-lived raw lookup?
- Can a stale identity, malformed packet, or failed resource request reach an
  application action without first returning an explicit failure?
- Does the evidence distinguish host facts, compile facts, and authorized target
  observations?

### Documentation synchronization

- Update the concept only for a durable accepted architectural decision.
- Update this roadmap only for sequence, gate definition, or artifact changes.
- Update PROGRESS when evidence, decision status, blocker, or current phase
  changes.
- Update ADRs when an owner accepts or revises a design choice.
- Update package documentation, headers, tests, and evidence together when a
  public candidate or release changes behavior.
- Do not copy assumptions into benchmark results or claim an unresolved budget
  as measured.

## Rollback and execution rules

Candidate work is isolated in separately linkable modules. Revert the candidate
commit range or omit its package from a consumer; do not rewrite released Core
history to recover from a candidate failure. No persistence or hardware state
migration is implied by this roadmap.

- Work one gate at a time. Do not create later-phase production folders before
  the preceding gate's required owner decision.
- Read the relevant ADR, closest AGENTS guidance, public contract, and current
  PROGRESS entry before changing a module.
- Preserve v0.1 behavior and unrelated user changes. Compile each contract pair
  and run its narrow behavior tests before dependent work.
- Stop for explicit decisions on ownership, collector barriers, engine failure,
  wire format, authentication, board selection, or resource budgets.
- Do not upload, transmit, erase, configure hardware, or claim runtime behavior
  without explicit authorization and recorded evidence.
- Update PROGRESS for live state and append dated history here when an accepted
  decision changes this sequence.

## Plan history

| # | Date | Reviewer | Changes made |
| ---: | --- | --- | --- |
| 1 | 2026-07-19 | Project owner | Expanded MicroWorld from a lifecycle/tick kernel into a lightweight UE-shaped microcontroller engine. |
| 2 | 2026-07-19 | Codex | Proposed modular Core/Managed/Connected concept with bounded optional GC. |
| 3 | 2026-07-19 | Project owner | Approved optional GC rationale and required flexibility because early engine requirements are still being discovered. |
| 4 | 2026-07-19 | Codex | Refined Connected into a Net overlay usable with Core or Managed ownership and wrote the multi-release implementation plan. |
| 5 | 2026-07-19 | Sceptic-critic pass | Removed Net-to-Engine coupling, added a separate bridge, required standard-vs-custom pointer evidence, made root failure explicit, deferred thread-safe pointers, and added Engine frame-failure gating. |
| 6 | 2026-07-19 | Architecture review | Verdict SOUND for roadmap use; added release-gate checks against God classes and dependency drift. |
| 7 | 2026-07-19 | Project owner | Approved the reviewed roadmap and authorized continuation to implementation-path selection. |
| 8 | 2026-07-19 | Phase 0 verification | Gate A passed from exact committed production sources: host build/tests/static checks/benchmark and both ESP32 compile-only consumers reproduced; PlatformIO Native was blocked at this checkpoint by missing GNU `g++` and was resolved by history entry 12. |
| 9 | 2026-07-19 | Phase 1 verification | Established `MicroWorld::Core` with 0.1 target compatibility, selected adjacent PlatformIO manifests from official 6.1 behavior, added negative-tested dependency/map gates, passed standalone CMake plus ESP32 consumers, and recorded Gate B evidence; the checkpoint's missing-`g++` limitation was resolved by history entry 12. |
| 10 | 2026-07-19 | Project owner | Removed raw `TUniquePtr::Release`; move is the only Phase 2 ownership transfer because a raw pointer cannot retain the injected resource and exact allocation block. |
| 11 | 2026-07-19 | Phase 2 verification | Implemented the adjacent Memory candidate with explicit resources, fallible unique/shared ownership, fixed containers, and bounded delegates; passed 27 host cases, benchmarks, dependency/profile checks, standalone no-exceptions/no-RTTI consumption, and ESP32-S3 compile evidence; a narrow ASan lifetime regression passed, while installed Windows ASan/UBSan runtimes left full-suite sanitizer evidence environment-limited; Gate C promotion and target margin remain owner decisions. |
| 12 | 2026-07-19 | Host toolchain verification | Installed WinLibs GCC 16.1.0 through WinGet for the current user, added `mingw64/bin` to the user `PATH`, and passed the PlatformIO Native Core consumer build and runtime probe with exit code zero. |
| 13 | 2026-07-19 | Project owner | Accepted Gate C for roadmap progression after paired Clang++ 20.1.8 ASan/UBSan Memory evidence passed 27/27; Memory remains experimental pending target-margin evidence. |
| 14 | 2026-07-19 | Phase 3 verification | Completed the Object package, generational handles, descriptor registry, roots, bounded incremental GC, consumers, tests, benchmarks, and profile gates; MSVC, strict GCC, and paired Clang ASan/UBSan passed 25/25; ESP32-S3 compile/link and package/map checks passed without upload; technical Gate D evidence is complete and owner acceptance remains pending. |
| 15 | 2026-07-19 | Documentation architecture cleanup | Archived the detailed design at `cf5d964`, separated durable decisions, executable sequence, and live progress, and removed duplicated speculative implementation detail. |
