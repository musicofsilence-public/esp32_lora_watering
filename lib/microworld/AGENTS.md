# MicroWorld Package

Inherits `../AGENTS.md`.

## Architecture

Released MicroWorld 0.1 is a platform-neutral runtime kernel. `FApplication`
guards a consumer composition root, `TWorld<N>` dispatches registered Actors,
`TActor<N>` aggregates registered Components, `FNetwork` provides an
independent policy-free subsystem boundary, and `FTickFunction` owns scheduling
state shared by all tickable types.

The approved pre-1.0 evolution keeps this deterministic Core usable by itself,
adds Managed ownership as an optional layer, and adds Net as an independent
overlay usable with either ownership tier. Portable dependency direction is
`Core <- Memory <- Object <- Engine`; Serialization and Net depend inward on
Core/Memory, and a separate integration boundary may depend on both Engine and
Net. Vendor ports depend on portable contracts and never reverse that
direction.

`lib/microworld` is the Core PlatformIO package and the physical
`microworld`/`MicroWorld::Core` CMake library. The Memory Gate C and Object
Gate D candidates use adjacent packages and manifests because PlatformIO
builds every source admitted by a selected library manifest. Profiles compose
packages; they never mutate Core's source set with feature macros. The released
`FNetwork` remains a Core lifecycle/tick boundary and is not the future Net
module.

## Core concepts

- Consumers own every concrete object; World and Actor registrations are
  bounded, non-owning, pointer-stable references.
- Lifecycle moves forward through constructed, playing, failed, or ended
  states. Begin failures become terminal and successful end is idempotent.
- One caller-supplied monotonic millisecond time source drives all scheduling.
- Actor, Component, and Network ticks remain independently configurable.
- Runtime failures return typed results; MicroWorld never logs, throws, reads
  hardware, or defines product policy.
- Managed memory, garbage collection, and dynamic Actor behavior are approved
  roadmap capabilities. Object APIs are candidates, not released 0.1 API;
  borrowed `U`/`A` names cannot be promoted or released before their gates.
- GC is optional per build but first-class when Managed is linked. Hardware,
  ISR, watchdog, transport-driver, and safety lifetimes remain deterministic
  and outside GC.
- Borrowed UE names must carry the promised semantics. `UObject`, `AActor`, and
  `UActorComponent` cannot enter the public API before managed identity,
  tracing, lifecycle, and destruction gates pass.

## Implementation contract

- Use `F`/`T`/`E`/`b` naming and intent-focused Doxygen contracts for functions
  and persistent state as required by `lib/AGENTS.md`.
- Keep lifecycle and tick paths bounded, single-pass, allocation-free, and free
  of container mutation.
- No ESP-IDF, FreeRTOS, Arduino, radio, valve, or tutorial dependency may enter
  the production package.
- All dynamic memory must use an injected resource with explicit capacity and
  failure. No portable module silently falls back to a platform heap.
- Gate each candidate release independently. Do not create later module
  scaffolding until the current behavior, dependency, and resource evidence is
  reviewed and accepted.
- Run package ownership and profile map checks whenever source placement,
  manifest contents, or target dependencies change.
- Treat `.claude/plans/microworld-mini-engine-roadmap.md` as the approved
  implementation sequence; public headers, tests, and release documentation
  remain authoritative for what is actually implemented.
- Treat `PROGRESS.md` as the sole live status record. Update it in the same
  commit when a MicroWorld implementation, gate, evidence, decision, blocker,
  or next milestone changes; reviewers reject omissions.

## Verification

- Verify with `cmake -S lib/microworld -B lib/microworld/build`,
  `cmake --build lib/microworld/build`, and
  `ctest --test-dir lib/microworld/build --output-on-failure`.
- Run the declaration-documentation and directory-coverage tools, format all
  C/C++ files with the repository policy, and compile the downstream consumer
  probes appropriate to the installed toolchains.
