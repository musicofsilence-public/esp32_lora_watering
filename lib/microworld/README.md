# MicroWorld 0.1.0

MicroWorld is a standalone C++17 lifecycle and primary-tick framework for
bounded embedded applications. It has no ESP32, RTOS, radio, transport, or
product-policy dependency.

## Ownership and lifetime

The consumer composition root owns every concrete object. `TWorld<N>` stores
non-owning Actor pointers, and `TActor<N>` stores non-owning Component pointers.
Each Actor can register with one World and each Component with one Actor.
Runtime objects cannot be copied or moved.

Declare Components before their Actor and Actors before their World. Reverse
destruction then removes the World first, followed by Actors and Components, so
no registered pointer outlives its storage.

## Lifecycle

`BeginPlay(now)` starts once. World startup visits Actors in registration order;
each Actor starts Components in registration order before its own hook.
`Advance(now)` accepts equal or increasing millisecond values. For every Actor,
due Components tick in registration order before the Actor's own primary tick.
`EndPlay()` visits Actors in reverse order; each Actor ends before its
Components, which end in reverse registration order. A successful `EndPlay()`
is idempotent.

Registration closes at `BeginPlay`. Duplicate, capacity, lifecycle, ownership,
and time errors are returned as `ERuntimeResult`; the framework does not log or
throw.

## Primary tick contract

`FTickConfiguration` defines whether an object can ever tick, whether it starts
enabled, and its minimum interval. Actor, Component, and Network schedules are
independent. Disabling an Actor does not suppress Component lifecycle or ticks.

Interval zero is due once per caller update. A first enabled tick, re-enable, or
enabled interval reset executes on the next `Advance` with delta zero. Later
delta values measure from that object's previous executed tick. A late tick
executes once and schedules its next deadline from actual execution time, so
there is no catch-up burst. Unrepresentable deltas saturate at
`DurationMilliseconds` maximum.

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

Build independently:

```sh
cmake -S lib/microworld -B <build-directory>
cmake --build <build-directory>
ctest --test-dir <build-directory> --output-on-failure
```

New CMake consumers link `MicroWorld::Core`. The physical `microworld` target
remains available for released 0.1 compatibility.

PlatformIO downstream probes consume the local 0.1.0 package without adding
platform dependencies to MicroWorld:

```sh
pio run -d lib/microworld/tests/consumer -e native
pio run -d lib/microworld/tests/consumer -e esp32-s3
pio run -d lib/microworld/tests/consumer -e esp32-s3-benchmark
```

The native PlatformIO environment requires a host GNU `g++` compiler on
`PATH`; it currently passes with WinLibs GCC 16.1.0. The ESP32-S3 environments
use PlatformIO's managed Espressif toolchain. These commands build only;
uploading or running the target benchmark is a separate explicitly authorized
hardware step.

The platform-neutral example is `examples/HostLifecycle/Main.cpp`. Performance
methods and evidence are in [docs/Performance.md](docs/Performance.md), with
host results in [benchmarks/Results/Host.md](benchmarks/Results/Host.md) and
ESP32-S3 compile/runtime status in
[benchmarks/Results/Esp32S3N16R8.md](benchmarks/Results/Esp32S3N16R8.md).

## Approved engine evolution

MicroWorld 0.1 remains the released deterministic lifecycle/tick API. The
approved pre-1.0 direction adds separately gated Memory, Object, Engine,
Serialization, and Net capabilities without making managed memory mandatory.
The adjacent Memory package is implemented as a Gate C candidate but is not
promoted or released; later capabilities remain roadmap work.

- [UE5-to-MicroWorld concept and semantic map](docs/UE5ConceptMap.md)
- [CMake/PlatformIO module packaging and Gate B/Gate C evidence](docs/ModulePackaging.md)
- [Profile resource budgets and evidence states](docs/ResourceBudgets.md)
- [Accepted architecture decision records](docs/decisions)
- [Porting and target-evidence obligations](docs/Porting.md)

## Verification status

The implementation passes its 31 host behavior cases, CTest integration,
dependency and profile-map gates, strict public-header compilation,
class-documentation check, and folder-guide coverage check. Standalone CMake
and exact-version ESP32-S3 PlatformIO Core consumers compile successfully, as
does the repository's existing firmware environment. The native PlatformIO
consumer compiles with WinLibs GCC 16.1.0 and returns exit code zero. Target
cycle, heap, and stack measurements remain blocked until an explicitly
authorized hardware run.

Maintained C/C++ files are formatted with the repository `clang-format` policy.
Public functions and persistent state carry intent-focused contracts explaining
their ownership, lifecycle, scheduling, or evidence role. Scoped `AGENTS.md`
files describe the architecture and concepts owned by every package directory.

## Known limitations

v0.1 has no runtime registration/removal, dynamic spawning, lookup, reflection,
garbage collection, event bus, transforms, protocol policy, tick groups,
prerequisites, parallel ticks, or real-time guarantee. Fixed capacity is chosen
by the consumer at compile time. Source compatibility before 1.0 is not
promised.
