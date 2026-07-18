# Package Documentation

Inherits `../AGENTS.md`.

## Architecture

Documentation is a read-only description of the released package contract.
`Style.md` owns naming and comment policy, `Porting.md` owns downstream
toolchain/measurement obligations, and `Performance.md` owns optimization
methodology. Runtime code never depends on prose.

## Concepts

- The public headers and behavior tests are authoritative for API semantics.
- Benchmark result files are authoritative only for the explicitly recorded
  environment and source state.
- Porting guidance separates compile evidence from observed target behavior.
- Package documentation may describe future work but must not make it part of
  the current API.

## Verification

Do not invent platform support or change API semantics in prose. Verify links,
versions, symbols, lifecycle ordering, ownership claims, and measured values
against headers, tests, build output, and result records.
