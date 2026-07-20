# Engine Host Evidence

Status: **host behavior and strict-consumer evidence recorded against source/test
commit `52055fc` on 2026-07-20. The Simple Timers milestone is an accepted
implementation candidate; target runtime margins remain unmeasured.**

## Tested commit

The Engine production source, tests, and standalone consumer verified by this
record are anchored to commit **`52055fc`**
(`test(microworld): prove timer mixed stable compaction and tail reuse`).
That commit adds the final mixed-stable-compaction regression and corrects two
`Timer.h` comments; it changes no timer logic. The prior correction
`98b53c6` established the accepted production behavior (explicit mode allowlist,
single-pass post-dispatch compaction, redundant destructor removed, handle
locality documented).

## Environment and verification

| Item | Recorded value |
| --- | --- |
| Host | Windows x64 |
| CMake | 4.0.2 |
| Generator | Visual Studio 17 2022 |
| MSVC | 19.44.35226.0, Release |
| MSVC test warning gate | `/W4 /WX /Zc:__cplusplus` |
| Exceptions / RTTI (production + tests) | `/EHs-c-` / `/GR-` |
| MSVC Engine tests | 55/55 passed |
| WinLibs GCC | 16.1.0 (MinGW-W64 ucrt-posix-seh) |
| LLVM Clang | 19.1.5 (Target: x86_64-pc-windows-msvc), against MSVC 14.44.35207 STL |
| Standalone Engine consumer | Built with `/W4 /WX /EHs-c- /GR-` and exited 0 |
| Four-package dependency check | Passed across 42 files |
| Engine class-documentation check | Passed across 18 files (`--require-doxygen --max-sentences 3`) |
| Managed profile map (host consumer) | Passed at 69,294 bytes |

## Corrected timer behavior

| Area | Recorded value |
| --- | --- |
| Timer behavior cases | 34 (one-shot, looping, cancellation, handles, capacity/invalid input, caller-supplied time, ordering incl. mixed stable compaction and tail reuse, dispatch mutation rules, allocation) |
| Lifecycle / registration / GC cases | 21 (unchanged) |
| Total Engine behavior cases | 55 |

`Schedule` accepts only `OneShot` and `Looping`; every other `ETimerMode`
(`None` and arbitrary casts such as `static_cast<ETimerMode>(3)`) is rejected
transactionally as `InvalidMode`. `Advance` clears completed one-shot slots in
place during dispatch and removes them all in a single stable post-dispatch
compaction pass, preserving the relative order of every survivor. The
`EngineTimerMixedStableCompactionPreservesSurvivorsAndTailReuse` regression
proves that two looping survivors around a removed one-shot keep their order
and that a replacement scheduled into the freed physical slot dispatches at the
logical insertion tail, not at the freed slot's original position.

`FTimerHandle` is documented as a plain {slot index, generation} pair local to
the issuing `TTimerManager`. Deleted move operations preserve the deliberately
simple application-owned manager lifetime/identity boundary; relocation would
not mechanically rewrite handles, so the rationale is ownership-explicitness,
not mechanical invalidation. The redundant explicit destructor loop was removed
because each `FTimerSlot` owns its `TDelegate` member and `TDelegate` destroys
its bound callable exactly once.

## Steady-state allocation

`EngineTimerOperationsPerformNoObservableAllocation` exercises Schedule, Advance
dispatch, Cancel, slot reuse, and looping operation under a process-wide
`operator new`/`delete` override. It asserts every Schedule, Advance, and
Cancel result, asserts callback counts and `TimerCount()` after each step,
actually dispatches the reused one-shot at its calculated deadline, and records
zero scalar, array, or aligned global allocations across the whole sequence.

## Exact verification commands

### Release tests (Core, Memory, Object, Engine)

```text
cmake -S lib/microworld -B <core-build> -DMICROWORLD_BUILD_TESTS=ON -DMICROWORLD_BUILD_BENCHMARKS=OFF -DMICROWORLD_BUILD_EXAMPLES=OFF
cmake --build <core-build> --config Release
ctest --test-dir <core-build> -C Release --output-on-failure

cmake -S lib/microworld-memory -B <memory-build> -DMICROWORLD_MEMORY_BUILD_TESTS=ON -DMICROWORLD_MEMORY_BUILD_BENCHMARKS=OFF
cmake --build <memory-build> --config Release
ctest --test-dir <memory-build> -C Release --output-on-failure

cmake -S lib/microworld-object -B <object-build> -DMICROWORLD_OBJECT_BUILD_TESTS=ON -DMICROWORLD_OBJECT_BUILD_BENCHMARKS=OFF
cmake --build <object-build> --config Release
ctest --test-dir <object-build> -C Release --output-on-failure

cmake -S lib/microworld-engine -B <engine-build> -DMICROWORLD_ENGINE_BUILD_TESTS=ON -DMICROWORLD_ENGINE_BUILD_BENCHMARKS=OFF
cmake --build <engine-build> --config Release
ctest --test-dir <engine-build> -C Release --output-on-failure
<engine-build>/Release/microworld_engine_tests.exe
```

### Strict GCC 16.1.0 timer-TU compile

```text
C:\Users\chorn\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin\g++.exe \
  -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -fno-exceptions -fno-rtti \
  -I lib/microworld/include \
  -I lib/microworld-memory/include \
  -I lib/microworld-object/include \
  -I lib/microworld-engine/include \
  -I lib/microworld/tests \
  -I lib/microworld-engine/tests \
  -c lib/microworld-engine/tests/EngineTimerManagerTests.cpp \
  -o EngineTimerManagerTests.gcc.obj
```

Result: exit 0, no warnings.

### Strict Clang 19.1.5 timer-TU compile

The VS-bundled LLVM Clang 19.1.5 targets `x86_64-pc-windows-msvc` and must use
the VS 2022 MSVC STL (`14.44.35207`); the VS 18 Insiders STL on this host
demands Clang 20+ and would reject any C++ TU including `<cstdint>` with
`error STL1000`, independent of source. The compile uses `-nostdinc++` plus the
VS 2022 STL include path:

```text
"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\bin\clang++.exe" \
  -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -fno-exceptions -fno-rtti \
  -nostdinc++ \
  -isystem "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include" \
  -I lib/microworld/include \
  -I lib/microworld-memory/include \
  -I lib/microworld-object/include \
  -I lib/microworld-engine/include \
  -I lib/microworld/tests \
  -I lib/microworld-engine/tests \
  -c lib/microworld-engine/tests/EngineTimerManagerTests.cpp \
  -o EngineTimerManagerTests.clang.obj
```

Result: exit 0, no warnings.

### Strict standalone Engine consumer

```text
cmake -S lib/microworld/tests/consumer -B <consumer-build> -DMICROWORLD_STANDALONE_ENGINE_CONSUMER=ON
cmake --build <consumer-build> --config Release
<consumer-build>/Release/microworld_engine_consumer.exe
```

Result: exit 0.

### Static checks

```text
python lib/microworld/tools/CheckDependencyBoundaries.py ^
  --package Core=lib/microworld ^
  --package Memory=lib/microworld-memory ^
  --package Object=lib/microworld-object ^
  --package Engine=lib/microworld-engine ^
  --exclude build --exclude .pio --exclude __pycache__

python lib/microworld/tools/CheckClassDocumentation.py ^
  --root lib/microworld-engine --exclude build --exclude .pio ^
  --exclude __pycache__ --require-doxygen --max-sentences 3

python lib/microworld/tools/CheckProfileMap.py ^
  --map <consumer-build>/microworld_engine_profile.map --profile Managed
```

Results: dependency-boundary check passed (4 packages, 42 files);
class-documentation check passed (18 files); Managed profile map check passed
(69,294 bytes).

Host behavior and compile evidence do not establish target runtime timing,
stack, heap, power, or physical-hardware behavior. No firmware upload, run, or
hardware measurement was performed.
