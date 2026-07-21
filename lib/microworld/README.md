# MicroWorld Core 0.1.0

MicroWorld Core is a standalone C++17 lifecycle and tick runtime for bounded
embedded applications. It has no ESP32, RTOS, transport, or product-policy
dependency.

Current development status is in [PROGRESS.md](PROGRESS.md).

## What Core provides

- `FApplication` guards a consumer composition root.
- `FTickFunction` owns bounded per-object scheduling: each object carries its
  own tick configuration and enable flags.
- Caller-supplied monotonic milliseconds drive scheduling. Late ticks run once,
  never in a catch-up burst.
- `FLifecycleGuard` and the `FTickable` contract express a forward-only
  begin/tick/end lifecycle without scattered boolean flags.
- Lifecycle, capacity, and time failures return `ERuntimeResult`.
- `MW_LOG` / `MW_LOG_MSG` give a bounded logging facade every package can use.

Core is lifecycle and tick **primitives** only. The managed World / Actor /
Component model lives in the Engine package (`UWorld` / `AActor` /
`UActorComponent`); Core retired its own duplicate Actor model in the Phase 1
consolidation. Consumers still own their concrete objects.

## Public headers

- `MicroWorld/Application.h`
- `MicroWorld/Lifecycle.h`
- `MicroWorld/Log.h`
- `MicroWorld/TickFunction.h`
- `MicroWorld/Tickable.h`
- `MicroWorld/Time.h`
- `MicroWorld/Version.h`

## Logging

`MicroWorld/Log.h` is a bounded logging facade owned by Core and usable from
every package. It has four levels — `Error`, `Warning`, `Log`, `Verbose` — and a
single process-global sink:

```cpp
using FLogSink = void (*)(ELogLevel Level, const char* Category, const char* Message);
```

Install one sink at startup with `SetLogSink`; the default sink is null, which
disables logging. There are two call macros:

```cpp
MW_LOG(Warning, "Net", "peer %u timed out", Index); // printf-style
MW_LOG_MSG(Log, "Boot", "runtime ready");           // already-formed message
```

Use `MW_LOG_MSG` for a runtime string that may itself contain `%`. Formatting
happens in a fixed caller-stack buffer (`MW_LOG_MESSAGE_CAPACITY`, default 128
bytes) via `vsnprintf` — no heap, no exceptions, no clock.

A **compile-time floor** `MW_LOG_MIN_LEVEL` (default `Log`) strips less important
call sites entirely: they expand to nothing, emit no code, keep no format or
category strings in flash, and never evaluate their arguments. Lower the floor
for a build with `-DMW_LOG_MIN_LEVEL=MW_LOG_LEVEL_Verbose`. The facade is
single-threaded; install the sink before the first log call.

## Build

```sh
cmake -S lib/microworld -B <build-directory>
cmake --build <build-directory>
ctest --test-dir <build-directory> --output-on-failure
```

CMake consumers link `MicroWorld::Core`. PlatformIO probes are available with:

```sh
pio run -d lib/microworld/tests/consumer -e native
pio run -d lib/microworld/tests/consumer -e esp32-s3
```

These commands build only. Uploading or running target firmware needs separate
hardware authorization.

## Next scope

The minimal managed Engine (`UWorld`, `AActor`, `UActorComponent`), the Simple
Timers milestone (`TTimerManager`), and the Simple Net milestone
(`FByteWriter`/`FByteReader`, `INetDriver`, `FNetManager`, `FHostLoopback`)
are accepted implementation candidates. The next scope is one ESP32-S3 example
that demonstrates the completed runtime on the existing ESP32-S3 configuration.

Core does not provide runtime spawn/destroy, timers, networking policy,
reflection, hardware abstraction, or a real-time guarantee.

## Evidence and details

- [Current status and evidence](PROGRESS.md)
- [Package architecture](docs/ModulePackaging.md)
- [Concept map](docs/UE5ConceptMap.md)
- [Resource rules](docs/ResourceBudgets.md)
- [Host benchmark](benchmarks/Results/Host.md)
- [ESP32-S3 compile evidence](benchmarks/Results/Esp32S3N16R8.md)
