# ADR 0001: Modular Runtime and Independent Net Overlay

- **Status:** Accepted
- **Date:** 2026-07-19
- **Decision owner:** Project owner

## Context

MicroWorld 0.1 is a deterministic lifecycle/tick kernel. The approved engine
vision also needs managed objects, GC, smart pointers, dynamic Actors,
serialization, networking, and platform ports. A single monolithic library
would impose unused memory and build cost on small applications and would make
platform and ownership policies difficult to separate.

## Decision

Evolve MicroWorld through separately linkable portable modules:

```text
Core <- Memory <- Object <- Engine
Core <- Serialization <- Net
Memory <----------------- Net
```

- **Core** remains independently usable and preserves the released deterministic
  lifecycle/tick model.
- **Managed** is an ownership tier composed from Core, Memory, Object, and
  Engine. It is optional per build.
- **Net** is an overlay usable with either Core or Managed. Net never depends on
  Engine.
- An optional integration target may depend on both Engine and Net to schedule
  a Net manager through Engine subsystem phases.
- Platform contracts stay portable. Vendor SDK implementations live in
  adjacent port packages and depend inward.
- Consumer composition roots own concrete resources, stores, subsystems,
  drivers, and adapters. No global mutable registry or platform singleton is
  introduced.

The released `lib/microworld` package is Core. CMake keeps the physical
`microworld` target for 0.1 compatibility and exposes `MicroWorld::Core` as its
namespaced alias. The released policy-free `FNetwork` remains Core and is not
the future Net module.

Future portable modules use adjacent repository roots, CMake targets, and
PlatformIO manifests. PlatformIO profiles select multiple packages through
`lib_deps`; they do not change Core sources with feature macros or a generated
manifest filter. Empty module packages are not created before their phase.
Exact reserved names and Phase 1 evidence are in
[ModulePackaging.md](../ModulePackaging.md).

This physical layout follows PlatformIO's documented rule that, after the
Library Dependency Finder selects a package, every source admitted by its one
manifest is built. Static archive mode controls linking but is not a source
profile mechanism.

## Consequences

- Core-only and Core-plus-Net applications do not pay for managed objects or
  GC.
- Managed applications gain honest `U`/`A` semantics after their gates pass.
- An extra integration boundary is preferable to a backward Net-to-Engine
  dependency.
- Builds and tests need a profile matrix and dependency checker.
- More targets increase maintenance cost, so each candidate release stops at an
  evidence gate.
- A profile has multiple package dependencies, which adds manifest/version
  coordination but keeps source ownership and resource attribution explicit.

## Alternatives considered

- **Literal miniature UE5 clone:** rejected because desktop assumptions and
  superficial compatibility would dominate constrained targets.
- **Unrelated helper libraries:** rejected because they would not provide a
  coherent engine mental model.
- **One configurable monolith:** rejected because feature-flag interaction,
  PlatformIO's single manifest source set, and resource attribution would be
  difficult to review.
- **Generated PlatformIO source filters:** rejected because scripted build
  mutation would become the module boundary and diverge from ordinary CMake
  consumption.

## Revisit triggers

- A later PlatformIO release gains a declarative per-environment package source
  profile that is simpler than adjacent manifests.
- A real application needs a dependency that violates the selected direction.
- Module boundaries duplicate substantial knowledge rather than separate
  responsibilities.
- A second distribution repository becomes simpler than maintaining adjacent
  packages in this repository.

Any revision requires a superseding ADR, profile map evidence, and migration
impact.
