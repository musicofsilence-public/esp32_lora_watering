# Package Documentation

Inherits `../AGENTS.md`.

## Architecture

Documentation describes the released package contract and clearly labelled
candidate and future work.
`Style.md` owns naming and comment policy, `Porting.md` owns downstream
toolchain/measurement obligations, `Performance.md` owns optimization
methodology, `UE5ConceptMap.md` distinguishes released and planned UE-inspired
semantics, `ResourceBudgets.md` owns measured/provisional resource facts and
budget rules, `ModulePackaging.md` owns physical module selection and build
evidence, and `decisions/` records accepted architectural direction. Runtime
code never depends on prose.

## Concepts

Document authority is separated by claim type so current state, released
behavior, measurements, and accepted direction cannot be confused.

- `../PROGRESS.md` is authoritative for current implementation and gate state.
- Public headers, behavior tests, and release metadata are authoritative for
  released behavior.
- Benchmark result files are authoritative only for the explicitly recorded
  measurements, environment, and source state.
- `ModulePackaging.md` owns physical package/build/map evidence;
  `ResourceBudgets.md` owns measured/provisional resource facts and budget
  rules; live gate state remains in `../PROGRESS.md`.
- ADRs own accepted direction; the concept and roadmap own strategy and
  execution sequence.
- Porting guidance separates compile evidence from observed target behavior.
- Package documentation may describe future work but must not make it part of
  the current API.
- Decision records are append-only explanations of accepted direction. A later
  decision supersedes an earlier record rather than rewriting its history.

## Verification

Do not invent platform support or change API semantics in prose. Verify links,
versions, symbols, lifecycle ordering, ownership claims, and measured values
against headers, tests, build output, and result records.
