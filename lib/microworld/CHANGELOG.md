# Changelog

## Unreleased

Memory and Object are implemented candidates; the minimal managed Engine and
the Simple Timers milestone are accepted implementation candidates. The bounded
`TTimerManager` fixed-capacity timer facility passed its final mixed
stable-compaction regression (34 timer cases, 55 total Engine cases): explicit
mode allowlist, single-pass post-dispatch compaction preserving multiple
survivors and appending reused slots at the logical tail, redundant destructor
removed, and handle locality documented. The next milestone is Simple Net.
Live state and exact evidence are recorded in [PROGRESS.md](PROGRESS.md).

## 0.1.0 - 2026-07-18

Initial Core release:

- `FApplication`, `TWorld<N>`, `TActor<N>`, and `FActorComponent` provide
  bounded deterministic lifecycle dispatch.
- `FTickFunction` provides caller-time scheduling, saturation, independent
  tick configuration, and no catch-up bursts.
- CMake/CTest, PlatformIO consumer probes, host tests, a host example, and
  benchmark harnesses are included.

The release passed 31 host behavior cases, strict public-header and
no-exceptions/no-RTTI consumers, and recorded ESP32-S3 compile evidence. See
the [Core evidence records](benchmarks/Results) for exact toolchain and build
facts.
