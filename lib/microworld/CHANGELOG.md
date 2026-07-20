# Changelog

## Unreleased

Memory and Object are implemented candidates; the minimal managed Engine, the
Simple Timers milestone, and the Simple Net milestone are accepted
implementation candidates. The bounded Net package added a byte reader/writer
over caller-owned `TSpan`, one non-blocking `INetDriver`, one caller-storage-
backed fixed-capacity `FNetManager` over a caller-supplied
`FNetPacketStorage`, normalized `Success`/`Full`/`Invalid`/`Unavailable`
semantics, safe rejection of invalid `{nullptr, nonzero}` backing spans,
transactional receive failures, and a deterministic `FHostLoopback` driver
(52 behavior cases including recorded-packet FIFO order, exact head retention
across driver backpressure, recovery, caller-storage wraparound reuse,
byte-reader valid empty `{nullptr, 0}` suffix view safety, host-loopback
null-destination-before-empty-queue `Invalid` rejection, and private
caller-owned packet storage observed only through the matching `FNetManager`
specialization, plus steady-state zero allocation; strict Core+Memory+Net
consumer and GCC 16/Clang 19 TU compiles clean). Net depends only on Core and
Memory; the dependency/profile checkers were corrected to drop stale
Serialization/Integration assumptions, require the Net archive, and reject
Object/Engine leakage into the Core+Net profile. The next milestone is one
ESP32-S3 example. Live state and exact evidence are recorded in
[PROGRESS.md](PROGRESS.md).

Removed the duplicate Core `TWorld`/`TActor`/`FActorComponent`/`FNetwork` actor
model; the managed Engine (`UWorld`/`AActor`/`UActorComponent`) is now the sole
World/Actor/Component API and Core is lifecycle/tick primitives only
(`FApplication`, `FTickFunction`, `FLifecycleGuard`, `FTickable`).

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
