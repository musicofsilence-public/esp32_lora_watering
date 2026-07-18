# Host Benchmark Result

Status: measured locally on 2026-07-18; suitable as a correctness-first host
baseline, not target approval.

| Field | Value |
|---|---|
| Host | Windows x64 |
| Compiler | MSVC 19.44.35226.0 |
| CMake | 4.0.2 |
| Configuration | Release (`/O2` through the MSVC CMake profile) |
| LTO | Off |
| Trials | 30 |
| Warm-up / measured updates | 1,000 / 10,000 per trial |
| Repository HEAD | `29ccdff21673e20d225d6fc5b9eb60a0aa8b8671` |
| Source state | Dirty: MicroWorld implementation is uncommitted |

The recorded HEAD does not identify the new source. Commit authorization has
now been granted, but these values remain the pre-commit implementation
baseline. Rerun the fixed benchmark from the resulting clean commit before
using its hash as release or tutorial-pin evidence.

## Object sizes

| Type | Bytes |
|---|---:|
| `FTickFunction` | 32 |
| `FActorComponent` | 56 |
| benchmark `TActor<4>` | 104 |
| `TWorld<8>` | 88 |
| `FNetwork` | 48 |
| `FPerformanceSample` | 40 |

## Dispatch profiles

| Workload | Median ns/update | p95 | Worst | Steady-state allocations |
|---|---:|---:|---:|---:|
| Disabled | 4 | 4 | 5 | 0 |
| All due | 11 | 14 | 15 | 0 |
| Mixed rate | 9 | 10 | 11 | 0 |
| Maximum capacity (8 Actors × 4 Components) | 200 | 248 | 285 | 0 |

Host scheduling and background load can affect nanosecond timing. These values
are a local relative baseline only; they do not establish ESP32 cycles, flash,
static RAM, or stack margin.
