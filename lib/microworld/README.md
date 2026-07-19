# MicroWorld Core 0.1.0

MicroWorld Core is a standalone C++17 lifecycle and tick runtime for bounded
embedded applications. It has no ESP32, RTOS, transport, or product-policy
dependency.

Current development status is in [PROGRESS.md](PROGRESS.md).

## What Core provides

- `FApplication` guards a consumer composition root.
- `TWorld<N>` registers non-owning Actors; `TActor<N>` registers non-owning
  Components.
- Registration is fixed-capacity and closes at `BeginPlay`.
- Begin and tick use registration order; shutdown uses reverse order.
- Components begin and tick before their Actor; Actors end before Components.
- Caller-supplied monotonic milliseconds drive scheduling. Late ticks run once,
  never in a catch-up burst.
- Lifecycle, capacity, ownership, and time failures return `ERuntimeResult`.

Consumers own concrete objects. Declare Components before their Actor and
Actors before their World so reverse destruction cannot leave a registered
pointer dangling.

## Public headers

- `MicroWorld/Application.h`
- `MicroWorld/Actor.h`
- `MicroWorld/ActorComponent.h`
- `MicroWorld/Lifecycle.h`
- `MicroWorld/Network.h`
- `MicroWorld/TickFunction.h`
- `MicroWorld/Tickable.h`
- `MicroWorld/Time.h`
- `MicroWorld/Version.h`
- `MicroWorld/World.h`

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

The minimal managed Engine (`UWorld`, `AActor`, `UActorComponent`) is an
accepted implementation candidate above Memory and Object. The next scope is
simple fixed-capacity Engine timers, with simple Net and an ESP32-S3 example
later.

Core does not provide runtime spawn/destroy, timers, networking policy,
reflection, hardware abstraction, or a real-time guarantee.

## Evidence and details

- [Current status and evidence](PROGRESS.md)
- [Package architecture](docs/ModulePackaging.md)
- [Concept map](docs/UE5ConceptMap.md)
- [Resource rules](docs/ResourceBudgets.md)
- [Host benchmark](benchmarks/Results/Host.md)
- [ESP32-S3 compile evidence](benchmarks/Results/Esp32S3N16R8.md)
