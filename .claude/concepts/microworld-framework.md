# MicroWorld Framework

> **Status — historical approved v0.1 concept.** This concept was implemented
> and released in commit `c54f3c4`. The released contract and evidence live in
> [MicroWorld README](../../lib/microworld/README.md),
> [CHANGELOG](../../lib/microworld/CHANGELOG.md), and
> [benchmark results](../../lib/microworld/benchmarks/Results/). It is not the
> active post-0.1 roadmap.

## Problem

The ESP32 learning guide currently mixes two different subjects: implementing a
portable framework and learning ESP32 firmware development. That makes the
tutorial harder to follow and prevents MicroWorld from having a stable,
independently testable API. MicroWorld needs its own implementation session,
release gate, and documentation before the ESP32 tutorial consumes it.

## Proposed Approach

Implement MicroWorld as an independently buildable C++17 library under
`lib/microworld/`. It remains in this repository initially to avoid premature
multi-repository versioning, but its package, build, tests, namespace, and public
headers must not depend on ESP32, ESP-IDF, FreeRTOS, E32, or the remote
controller application.

MicroWorld v0.1 provides only the reusable concepts already required by the
remote controller:

- `FApplication` — lifecycle boundary with guarded `BeginPlay`, `Advance`, and
  `EndPlay`;
- `TWorld<MaxActors>` — deterministic owner/dispatcher for a fixed, bounded
  Actor set;
- `TActor<MaxComponents>` — high-level entity with explicit Component
  registration;
- `FActorComponent` — reusable behavior registered with one Actor;
- `FNetwork` — lifecycle/tick boundary for an application-defined network
  subsystem, without UART, radio, framing, authentication, or product messages;
- `FTickFunction` — the shared configurable ticking mechanism used independently
  by every Actor and Component.

Public C++ naming follows an embedded adaptation of UE5 style: `F` prefixes
non-UObject classes/structs, `T` prefixes templates, `E` prefixes enums, and `b`
prefixes booleans. Scalar aliases have no aggregate prefix and spell their
units, such as `TimePointMilliseconds`. Types are not called `AActor` or
`UActorComponent` because MicroWorld has no UObject inheritance, reflection, or
garbage collection. Namespaces, public types, enum values, methods, and matching
public header names use PascalCase; the package directory remains
`lib/microworld`.

The simplified UE5-inspired tick contract is:

- `bCanEverTick`, `bStartWithTickEnabled`, and an individual tick interval;
- runtime enable/disable and interval changes;
- interval zero means every World update;
- changing the interval does not enable a disabled tick;
- Actor and Component ticks are independent;
- disabling an Actor tick does not disable its Components;
- each due object ticks at most once per World update, with no catch-up burst;
- tick context reports monotonic time and elapsed time since that object's
  previous executed tick;
- lifecycle runs independently from tick enablement.

Only the dispatcher supplies monotonic time. Tick setters stage configuration
without accepting their own timestamps, and the next `Advance(Now)` returns an
explicit tick/no-tick/error decision. Runtime Actors, Components, Worlds, and
Networks are non-copyable/non-movable; each Actor can register with one World
and each Component with one Actor.

The library uses deterministic lifetimes, fixed-capacity/non-owning registration,
explicit errors, no exceptions or RTTI-dependent design, and no steady-state
heap allocation. It deliberately excludes reflection, garbage collection,
transforms, dynamic spawning, string lookup, tick groups, tick prerequisites,
parallel ticking, service location, event buses, and protocol policy.

Optimization is evidence-driven rather than speculative. The implementation
must inventory embedded techniques, measure size and runtime cost, then retain
only useful changes: fixed storage, single-pass dispatch, early disabled-tick
skips, no allocation/container mutation in tick paths, `noexcept`, bounded stack
use, and compiler size/LTO options where the target evidence supports them.
Virtual dispatch, 64-bit time, ordinary booleans, and readable control flow stay
unless benchmarks prove a material problem. Baselines include object sizes,
flash/static RAM, task or thread stack margin, cycles per update, and
steady-state allocations; unexplained regressions block release.
An executable ESP32-S3 benchmark consumer runs fixed disabled/all-due/mixed/
maximum-capacity workloads with repeatable sampling. Compile/map evidence is
always required; observed cycle/heap/stack output requires explicit upload/run
authorization and cannot be inferred from a successful build.

Every class declaration receives a concise Doxygen-style comment immediately
above it: one to three sentences describing purpose, ownership/lifetime, or a
critical invariant. Lifecycle, ownership, error, and tick-contract methods also
receive short contract comments; comments do not narrate syntax.

Every directory created for the package receives a scoped `AGENTS.md`. Parent
files define shared rules; child files add only local purpose, allowed
dependencies, naming/documentation requirements, hot-path constraints, and the
narrow verification command for that directory.

The framework implementation session delivers:

- public API and source;
- standalone CMake/CTest host build;
- tests for lifecycle, registration, independent ticking, timing, capacity, and
  failure behavior;
- documented performance baselines and an optimization report;
- one small host example;
- API/style/porting documentation;
- a concise `AGENTS.md` in every package directory;
- a versioned package manifest and release checklist.

## Open Questions

None for v0.1 planning. Moving MicroWorld to a separate Git repository is
deferred until a second real application proves that extraction is useful.

## Decisions Log

- 2026-07-18: Name the framework **MicroWorld** and use namespace
  `MicroWorld` — the product remains a separate consumer.
- 2026-07-18: Borrow UE5 lifecycle and ownership vocabulary without reproducing
  UObject, rendering, transforms, or dynamic engine machinery.
- 2026-07-18: Actors and Components each receive an independent configurable
  primary tick — enablement and interval are per object.
- 2026-07-18: Split framework implementation from the ESP32 tutorial —
  MicroWorld is implemented and released in a different session.
- 2026-07-18: Keep the first package in this repository but independently
  buildable — extraction remains possible without adding current overhead.
- 2026-07-18: Canonical time enters only through dispatcher Advance calls, and
  non-owning registrations enforce single ownership plus pointer-stable runtime
  objects.
- 2026-07-18: Adapt UE5 naming honestly: `F`/`T`/`E`/`b` prefixes and PascalCase,
  without misleading `A`/`U` prefixes for non-UObject types.
- 2026-07-18: Treat optimization as a measured release activity, require concise
  class contracts, and place scoped `AGENTS.md` guidance in every created
  package directory.
