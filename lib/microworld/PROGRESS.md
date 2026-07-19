# MicroWorld Progress

This is the sole live status record for MicroWorld; any implementation, gate,
evidence, decision, blocker, or next-milestone change must update it in the
same commit.

## Current checkpoint

**2026-07-19:** Gate D technical evidence is complete and owner acceptance is
pending. Phase 4 (Engine / Gate E) has not started.

## Gate state

| Gate | Technical evidence | Owner decision | Promotion/release state |
| --- | --- | --- | --- |
| A Baseline | Complete | Accepted | Baseline available |
| B Modules | Complete | Accepted | Core module boundary available |
| C Memory | Complete | Accepted for roadmap progression | Memory remains experimental pending accepted target margins |
| D Objects | Complete | Pending | Object candidate is not released |
| E Engine | Not started | No decision | Not available |
| F Net | Not started | No decision | Not available |
| G Ports | Not started | No decision | Not available |
| H Release | Not started | No decision | Not available |

Technical evidence, owner approval, and promotion are separate facts. A passed
test or compile probe does not itself promote an API or port.

## Delivered

- **Phase 0 / Core baseline:** released deterministic lifecycle and primary-tick
  kernel in commit `c54f3c4`.
- **Phase 1 / Modules:** Core module boundary and adjacent-package strategy in
  combined implementation checkpoint `e1e7b75`.
- **Phase 2 / Memory:** explicit resources, bounded utilities, ownership, and
  delegates in combined implementation checkpoint `e1e7b75`; Gate C remains
  experimental pending target margins.
- **Phase 3 / Object and GC:** generational handles, descriptors, roots,
  object store, and bounded incremental collection in combined implementation
  checkpoint `e1e7b75`.
- **Gate D metadata:** `cf5d964` records Gate D progress metadata and
  documentation; it is a roadmap archive anchor, not the Object implementation.

Commit hashes are `git show` anchors, not links or release-status claims.

## Verification

| Area | Recorded verification | Qualification |
| --- | --- | --- |
| Core | 31 behavior cases and 5 CTest gates | Released Core 0.1 evidence |
| Memory | 27 cases, including paired Clang 20 ASan/UBSan | Candidate evidence; target margins unresolved |
| Object | 25 final cases under MSVC Release, strict GCC 16 without exceptions/RTTI, and paired Clang 20 ASan/UBSan | Candidate evidence; Gate D owner decision pending |
| Packaging | CMake, PlatformIO, package, dependency, and map checks pass | Build/map evidence only |
| ESP Object image | 20,172 bytes RAM and 198,877 bytes flash | Compile-only whole-image evidence |

No target upload, runtime, timing, stack, heap, radio, or physical-hardware
claim is recorded for Memory or Object. Compile success does not establish any
of those properties.

## Open decisions and risks

- Owner acceptance of Gate D and its managed-object promotion conditions.
- Accepted target margins and authorized runtime evidence for candidate profiles.
- Gate E engine frame-failure ADR before Engine work begins.
- Exact STM32 and Pico reference targets, SDKs, toolchains, and budgets later.
- Net transport, wire format, threat model, authentication, and replication
  scope later.

## Next milestone

Owner Gate D review. Only if accepted, begin Gate E with the engine
frame-failure-semantics ADR; do not start Engine production work first.

## Reading this status

- **Complete technical evidence** means the recorded check set passed with its
  stated qualifications; it is not an owner approval or release claim.
- **Experimental** means target margin or other promotion conditions remain
  unresolved even when host and compile evidence exists.
- **Candidate** means adjacent package work exists but is outside the released
  Core 0.1 contract.
- **Not started** means no production phase is authorized by this checkpoint.
- Read the linked evidence before repeating a measurement or changing a gate.

## Evidence index

- [Released Core README](README.md) and [CHANGELOG](CHANGELOG.md)
- [Module packaging Phase 3 evidence](docs/ModulePackaging.md#phase-3-object-evidence)
- [Resource facts and budget rules](docs/ResourceBudgets.md)
- [UE5 semantic map](docs/UE5ConceptMap.md)
- [Core host evidence](benchmarks/Results/Host.md) and
  [Core ESP32 compile evidence](benchmarks/Results/Esp32S3N16R8.md)
- [Memory host evidence](../microworld-memory/benchmarks/Results/Host.md) and
  [Memory ESP32 compile evidence](../microworld-memory/benchmarks/Results/Esp32S3N16R8.md)
- [Object host evidence](../microworld-object/benchmarks/Results/Host.md) and
  [Object ESP32 compile evidence](../microworld-object/benchmarks/Results/Esp32S3N16R8.md)
- [Architecture decision records](docs/decisions/)
- [Active concept](../../.claude/concepts/microworld-mini-engine-roadmap.md)
  and [execution roadmap](../../.claude/plans/microworld-mini-engine-roadmap.md)

## Checkpoint history

These immutable snapshots record dated milestones. The current gate table above
supersedes them for live status.

| Date | Snapshot |
| --- | --- |
| 2026-07-19 | Gate A baseline reproduced from exact committed production sources. |
| 2026-07-19 | Gate B Core/package boundary and consumer evidence recorded. |
| 2026-07-19 | Gate C technical evidence completed; owner accepted roadmap progression while Memory remained experimental. |
| 2026-07-19 | Gate D technical evidence completed; owner acceptance remained pending. |
