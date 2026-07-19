# Consumer Entry Points

Inherits `../AGENTS.md`.

## Architecture

`NativeMain.cpp` and `Esp32Main.cpp` are Core probes.
`MemoryNativeMain.cpp` and `MemoryEsp32Main.cpp` exercise the Core+Memory
profile through one shared public-API probe. `ObjectNativeMain.cpp` and
`ObjectEsp32Main.cpp` exercise the Core+Memory+Object profile through fixed
storage, root, weak-reference, and collection APIs. `Esp32BenchmarkMain.cpp`
remains the only target runtime harness. The parent project guarantees that
only one entry point is linked per environment.

## Concepts and boundaries

- Every entry point asserts the exact MicroWorld 0.1.0 public version.
- Basic probes instantiate a public World type without reaching into private
  implementation. The native probe also calls one out-of-line tick primitive so
  linker maps contain positive Core archive evidence.
- Memory probes construct fixed-resource unique/shared ownership, bounded
  containers, and delegates; virtual resource destruction forces positive
  Memory archive evidence.
- Object probes retain one root through collection, then prove weak expiry
  after unrooted reclamation through public APIs.
- The benchmark owns ESP-IDF counters, serial output, heap/stack sampling, and
  fixed workloads so target dependencies remain outside MicroWorld.
- Benchmark validation compares cumulative tick counts after every trial to
  expose semantic drift.

## Documentation and verification

Document functions, workload constants, result fields, and accumulated state by
the evidence they produce. Keep probes minimal and benchmark work fixed and
bounded.
- Verify with `pio run -d lib/microworld/tests/consumer -e native`,
  `-e esp32-s3`, `-e esp32-s3-memory`, `-e esp32-s3-object`, or
  `-e esp32-s3-benchmark`.
