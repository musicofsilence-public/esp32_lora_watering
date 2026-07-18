# Consumer Entry Points

Inherits `../AGENTS.md`.

## Architecture

`NativeMain.cpp` is the host compile/link probe, `Esp32Main.cpp` is the minimal
ESP-IDF compile/link probe, and `Esp32BenchmarkMain.cpp` is the only target
runtime harness. The parent project guarantees that only one entry point is
linked per environment.

## Concepts and boundaries

- Every entry point asserts the exact MicroWorld 0.1.0 public version.
- Basic probes instantiate a public World type without reaching into private
  implementation.
- The benchmark owns ESP-IDF counters, serial output, heap/stack sampling, and
  fixed workloads so target dependencies remain outside MicroWorld.
- Benchmark validation compares cumulative tick counts after every trial to
  expose semantic drift.

## Documentation and verification

Document functions, workload constants, result fields, and accumulated state by
the evidence they produce. Keep probes minimal and benchmark work fixed and
bounded.
- Verify with `pio run -d lib/microworld/tests/consumer -e native`,
  `-e esp32-s3`, or `-e esp32-s3-benchmark`.
