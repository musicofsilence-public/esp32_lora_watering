# MicroWorld Module Packaging

MicroWorld uses separately linkable modules and separately manifested
PlatformIO packages. Phase 1 validates the Core package and the checks that
later module candidates must pass; it does not create empty future packages.
Current gate state is in [PROGRESS.md](../PROGRESS.md).

## Why packages are separate

PlatformIO 6.1 gives one library manifest one `build` object. Its `srcDir` and
`srcFilter` choose the package's source set, and the Library Dependency Finder
builds every source admitted by that manifest after selecting the library.
`libArchive` controls whether those objects enter a static archive; it does not
select different module source sets per consumer environment.

MicroWorld therefore does not use feature macros or generated source filters as
its ownership boundary. The released `MicroWorld` package is Core; adjacent
Memory and Object packages now exist as candidates. Engine, Serialization, Net,
Integration, and Platform packages remain future work.

Official behavior used for this decision:

- [PlatformIO library manifest](https://docs.platformio.org/en/stable/manifests/library-json/index.html)
- [`srcDir`](https://docs.platformio.org/en/stable/manifests/library-json/fields/build/srcdir.html)
  and
  [`srcFilter`](https://docs.platformio.org/en/stable/manifests/library-json/fields/build/srcfilter.html)
- [`includeDir`](https://docs.platformio.org/en/stable/manifests/library-json/fields/build/includedir.html)
- [`libArchive`](https://docs.platformio.org/en/stable/manifests/library-json/fields/build/libarchive.html)
- [Library Dependency Finder](https://docs.platformio.org/en/stable/librarymanager/ldf.html)

## Target and package names

The table records package presence only. [PROGRESS.md](../PROGRESS.md) owns
live gate and promotion state.

| Module | CMake target | PlatformIO package | Repository root | State |
| --- | --- | --- | --- | --- |
| Core | `MicroWorld::Core` | `MicroWorld` | `lib/microworld` | Released package |
| Memory | `MicroWorld::Memory` | `MicroWorldMemory` | `lib/microworld-memory` | Existing candidate package |
| Object | `MicroWorld::Object` | `MicroWorldObject` | `lib/microworld-object` | Existing candidate package |
| Engine | `MicroWorld::Engine` | `MicroWorldEngine` | `lib/microworld-engine` | Reserved; package absent |
| Serialization | `MicroWorld::Serialization` | `MicroWorldSerialization` | `lib/microworld-serialization` | Reserved; package absent |
| Net | `MicroWorld::Net` | `MicroWorldNet` | `lib/microworld-net` | Reserved; package absent |
| Engine-Net bridge | `MicroWorld::EngineNet` | `MicroWorldEngineNet` | `lib/microworld-integration` | Reserved; package absent |
| Platform contracts | `MicroWorld::Platform` | `MicroWorldPlatform` | `lib/microworld-platform` | Reserved; package absent |

The physical CMake target `microworld` remains available for 0.1 compatibility;
`MicroWorld::Core` aliases the same static library. The released `FNetwork`
also remains Core because it is a policy-free lifecycle/tick boundary. It is
not the future Net package, `FNetManager`, or `INetDriver`.

## Profile composition

Profiles are dependency bundles selected by the consumer:

| Profile | Packages |
| --- | --- |
| Core | Core |
| Memory | Core, Memory |
| Object | Core, Memory, Object |
| Core+Net | Core, Memory, Serialization, Net |
| Managed | Core, Memory, Object, Engine |
| Managed+Net | Core, Memory, Object, Engine, Serialization, Net |
| Managed+Net+Integration | Managed+Net plus Engine-Net bridge |

A CMake consumer links namespaced targets. A PlatformIO consumer lists each
selected package in `lib_deps`; local development uses one `symlink://` entry
per package. A profile name never changes which sources the Core manifest
admits.

## Automated gates

`CheckDependencyBoundaries.py` receives explicit `MODULE=PATH` package
ownership. It rejects:

- a package that contains another module's public/source folder;
- a dependency that points outward or backward;
- non-standard SDK/third-party headers in portable packages.

`CheckProfileMap.py` requires positive Core archive evidence and rejects archive,
path, and public-symbol markers for modules outside the selected profile. Both
tools run deterministic negative self-tests before checking repository output.

## Phase 1 evidence

Gate B was exercised on 2026-07-19 with no upload or target execution:

- CMake 4.0.2 / MSVC 19.44.35226.0 configured and built all Core targets.
- The standalone downstream CMake consumer linked `MicroWorld::Core` and
  returned success with exceptions and RTTI disabled.
- CTest passed five tests: the existing executable containing 31 behavior
  cases, dependency-checker self-test, live dependency check, map-checker
  self-test, and live host Core map check.
- `pio pkg pack` validated `library.json` and created
  `MicroWorld-0.1.0.tar.gz` in an ignored build directory.
- PlatformIO 6.1.19 built the ESP32-S3 Core probe at 20,156 bytes RAM and
  191,973 bytes flash.
- The ESP32-S3 benchmark probe built at 20,156 bytes RAM and 197,001 bytes
  flash. Its first concurrent build attempt stopped in ESP-IDF mbedTLS; an
  immediate isolated rerun passed, identifying resource contention rather than
  a repeatable source failure.
- Host and ESP32 maps passed the Core profile check and contained no unselected
  markers for Memory, Object, Engine, Serialization, Net, or Integration.
- PlatformIO's native environment compiled with WinLibs GCC 16.1.0 after its
  `mingw64/bin` directory was installed on the user `PATH`; the generated
  `program.exe` returned exit code zero.

These results establish build/package separation only. They do not claim
runtime timing, stack, heap, or hardware behavior.

## Phase 2 Memory evidence

Gate C candidate integration was exercised on 2026-07-19 without upload or
target execution:

- the standalone CMake consumer linked `MicroWorld::Core` and
  `MicroWorld::Memory`, compiled with exceptions and RTTI disabled, and returned
  success;
- its Memory profile map contained both physical archives, excluded all later
  module markers, and passed the extended negative-tested checker;
- the host Memory benchmark passed semantic counters and measured zero global
  allocations for fixed-resource pointers, containers, and delegates;
- the successful exceptions-enabled standard shared prototype attributed its
  one global allocation but did not attempt unsafe deliberate OOM;
- PlatformIO composed the Core and Memory manifests for `esp32-s3-memory` and
  built a complete image at 20,156 bytes RAM and 194,457 bytes flash;
- the first ESP attempt exposed flags leaking into ESP-IDF C sources; scoping
  C++17/strict/no-exception/no-RTTI flags to the representative C++ component
  corrected the integration, after which compile/link passed;
- the initial installed-runtime mismatch was resolved by pairing the Visual
  Studio 18 Insiders Clang 20 compiler with its matching sanitizer runtime;
- the full Memory suite then passed all 27 tests under AddressSanitizer and
  UndefinedBehaviorSanitizer;
- no target runtime, heap, stack, timing, or physical behavior was claimed.

The owner accepted Gate C for roadmap progression, while Memory remains
experimental pending accepted target margins. These results do not promote a
release or establish an absolute target budget. The working paired host
sanitizer toolchain is now recorded.

## Phase 3 Object evidence

Executable source is anchored to commit `e1e7b75`. See the exact
[host Object evidence](../../microworld-object/benchmarks/Results/Host.md) and
[ESP32 compile evidence](../../microworld-object/benchmarks/Results/Esp32S3N16R8.md).
Gate D candidate integration was exercised on 2026-07-19 without upload or
target execution:

- the standalone MSVC consumer linked `MicroWorld::Core`,
  `MicroWorld::Memory`, and `MicroWorld::Object`, compiled with exceptions and
  RTTI disabled, and returned success;
- its 43,939-byte link map contained all three physical archives plus
  `FObjectStore` and `FGarbageCollector`, excluded later module markers, and
  passed the `Object` profile check;
- the fixed 64-node benchmark built and returned success for equivalent full
  and incremental collection: 32 rooted chain nodes survived, an unreachable
  32-node cycle was reclaimed, and both modes performed 97 semantic operations;
- full collection completed in one call, while the `{2, 4, 8}` incremental
  root/mark/sweep budget completed in 15 slices with no slice exceeding 12
  operations;
- the benchmark used 8,192 bytes of object-slot storage, 2,048 bytes of slot
  metadata, 8 bytes of root storage, and 512 bytes of collector worklist
  storage. Each object occupied 64 payload bytes in a 128-byte slot; after
  collection the 32 live objects reported 2,048 payload bytes and 2,048 bytes
  of equal-slot internal fragmentation;
- `pio pkg pack` validated the adjacent Core, Memory, and Object manifests and
  produced three package archives;
- the existing PlatformIO Native Core consumer also rebuilt with WinLibs GCC
  16.1.0 and returned success;
- PlatformIO 6.1.19 resolved the local profile as three adjacent `symlink://`
  packages and built the ESP32-S3 Object image at 20,172 bytes RAM and 198,877
  bytes flash;
- the 4,589,127-byte ESP32 link map passed the same `Object` profile and
  required-symbol checks.
- optimized MSVC Release and strict GCC 16 builds each passed all 25 Object
  tests with exceptions and RTTI disabled;
- the paired Clang 20 AddressSanitizer and UndefinedBehaviorSanitizer build
  passed all 25 Object tests; its matching runtime directory was present on
  `PATH`;
- the tests cover root ownership, cycles, generation retirement, destruction
  and callback reentry, descriptor ownership, cross-store reference rejection,
  exact slice budgets, and collector cancellation.

Benchmark elapsed values are host-only observations. The ESP32 results are
compile/link and static-size evidence; no target timing, stack, heap, runtime,
or hardware behavior is claimed. For this dated source checkpoint, the
technical Gate D evidence set is complete.

## Verification

```sh
cmake -S lib/microworld -B lib/microworld/build/host
cmake --build lib/microworld/build/host --config Release
ctest --test-dir lib/microworld/build/host -C Release --output-on-failure

cmake -S lib/microworld/tests/consumer \
  -B lib/microworld/build/consumer \
  -DMICROWORLD_STANDALONE_CONSUMER=ON
cmake --build lib/microworld/build/consumer --config Release

python lib/microworld/tools/CheckDependencyBoundaries.py \
  --package Core=lib/microworld
python lib/microworld/tools/CheckProfileMap.py \
  --map <linker-map> --profile Core

cmake -S lib/microworld-memory -B lib/microworld-memory/build/gate-c
cmake --build lib/microworld-memory/build/gate-c --config Release \
  --target microworld_memory_benchmark

cmake -S lib/microworld/tests/consumer \
  -B lib/microworld-memory/build/gate-c-consumer \
  -DMICROWORLD_STANDALONE_MEMORY_CONSUMER=ON
cmake --build lib/microworld-memory/build/gate-c-consumer \
  --config Release --target microworld_memory_consumer

python lib/microworld/tools/CheckProfileMap.py \
  --map <memory-linker-map> --profile Memory

cmake -S lib/microworld-object \
  -B lib/microworld-object/build/gate-d
cmake --build lib/microworld-object/build/gate-d \
  --config Release --target microworld_object_benchmark

cmake -S lib/microworld/tests/consumer \
  -B lib/microworld-object/build/gate-d-consumer \
  -DMICROWORLD_STANDALONE_OBJECT_CONSUMER=ON
cmake --build lib/microworld-object/build/gate-d-consumer \
  --config Release --target microworld_object_consumer

python lib/microworld/tools/CheckProfileMap.py \
  --map <object-linker-map> --profile Object \
  --require FObjectStore --require FGarbageCollector

pio pkg pack lib/microworld --output <ignored-build-directory>
pio pkg pack lib/microworld-memory --output <ignored-build-directory>
pio pkg pack lib/microworld-object --output <ignored-build-directory>
pio run -d lib/microworld/tests/consumer -e esp32-s3
pio run -d lib/microworld/tests/consumer -e esp32-s3-memory
pio run -d lib/microworld/tests/consumer -e esp32-s3-object
pio run -d lib/microworld/tests/consumer -e esp32-s3-benchmark
```
