# Performance and Optimization

## Measurement contract

MicroWorld uses fixed storage, cached registration counts, early disabled-tick
skips, one-pass Actor/Component dispatch, no tick-path mutation, `const`, and
`noexcept`. These low-risk choices are retained because they also enforce the
bounded runtime contract.

The host benchmark runs four fixed workloads: disabled, all-due, mixed-rate,
and maximum capacity. Each profile warms 1,000 updates, measures 10,000 updates,
and repeats 30 trials. It reports public object sizes, median, p95, worst
nanoseconds/update, and steady-state allocation count. Host results are recorded
in `benchmarks/Results/Host.md`.

The ESP32-S3 consumer implements the same workload and emits one CSV row per
trial with cycle-counter overhead, CPU frequency, heap delta, and stack
high-water mark. Its compile-only build generates an ESP-IDF linker map and
PlatformIO flash/static-RAM report. Runtime cycle, heap, and stack evidence
still requires an explicitly authorized target run.
`benchmarks/Results/Esp32S3N16R8.md` records both the compile evidence and that
authorization boundary.

## Optimization decisions

Retained:

- compile-time capacities and non-owning pointer arrays;
- single-pass registration-order dispatch with cached counts;
- at most one call per object/update and no catch-up loop;
- early return for disabled/not-due ticks;
- no steady-state framework allocation or container mutation;
- saturated 64-bit time arithmetic and ordinary boolean state.

Rejected for v0.1:

- bit-packed flags and narrowed monotonic time, which weaken clarity or timing
  range without a failing budget;
- template replacement of all virtual hooks, which would remove heterogeneous
  bounded lists;
- custom allocators, because framework lifecycle and tick paths allocate
  nothing.

Deferred pending target evidence:

- `-Os` versus `-O2`;
- link-time optimization;
- any layout or dispatch micro-optimization.

No semantic change may be justified by host-only timing. An unexplained
regression over 10% outside the measured noise envelope blocks release.
