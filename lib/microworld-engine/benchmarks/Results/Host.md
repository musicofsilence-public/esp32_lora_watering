# Engine Host Evidence

Status: **host behavior and strict-consumer evidence on 2026-07-19 for the
corrected Simple Timers milestone. Engine remains an accepted candidate;
target runtime margins remain unmeasured.**

## Source boundary

Production source, tests, and the standalone Engine consumer are anchored to
the corrected Simple Timers milestone. The minimal managed Engine evidence
(lifecycle, registration, and garbage collection) is unchanged from the
prior milestone; only the timer facility and its tests were corrected.

## Environment and verification

| Item | Recorded value |
| --- | --- |
| Host | Windows x64 |
| CMake | 4.0.2 |
| Generator | Visual Studio 17 2022 |
| MSVC | 19.44.35226.0, Release |
| MSVC test warning gate | `/W4 /WX /Zc:__cplusplus` |
| Exceptions / RTTI (production + tests) | `/EHs-c-` / `/GR-` |
| MSVC Engine tests | 54/54 passed |
| WinLibs GCC | 16.1.0 (MinGW-W64 ucrt-posix-seh) |
| GCC timer-TU warning gate | `-std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -fno-exceptions -fno-rtti` |
| GCC strict compile of `EngineTimerManagerTests.cpp` | Passed |
| GCC strict compile of `Timer.h` consumer surface | Passed |
| Standalone Engine consumer | Built with `/W4 /WX /EHs-c- /GR-` and exited 0 |
| Four-package dependency check | Passed across 42 files |
| Engine class-documentation check | Passed across 18 files (`--require-doxygen --max-sentences 3`) |

## Corrected timer behavior

| Area | Recorded value |
| --- | --- |
| Timer behavior cases | 33 (one-shot, looping, cancellation, handles, capacity/invalid input, caller-supplied time, ordering, dispatch mutation rules, allocation) |
| Lifecycle / registration / GC cases | 21 (unchanged) |
| Total Engine behavior cases | 54 |

The corrected `TTimerManager` rejects every `ETimerMode` value except `OneShot`
and `Looping` (including `ETimerMode::None` and arbitrary casts such as
`static_cast<ETimerMode>(3)`) with a transactional `InvalidMode` result.
`Advance` clears completed one-shot slots in place during dispatch and removes
them all in a single stable post-dispatch compaction pass, so dispatch is
O(active + removed) total rather than one linear search and shift per fired
one-shot. The redundant explicit destructor loop was removed because each
`FTimerSlot` owns its `TDelegate` member and `TDelegate` destroys its bound
callable exactly once.

`FTimerHandle` is documented as local to the issuing `TTimerManager`: it is a
plain {slot index, generation} pair with no embedded manager identity. The
deleted move operations are documented by that handle-locality and
uniquely-owned-callback rationale, not by a slot-address-stability claim.

## Steady-state allocation

`EngineTimerOperationsPerformNoObservableAllocation` exercises Schedule,
Advance dispatch, Cancel, slot reuse, and looping operation under a process-
wide `operator new`/`delete` override. It asserts every Schedule, Advance, and
Cancel result, asserts callback counts and `TimerCount()` after each step,
actually dispatches the reused one-shot at its calculated deadline, and
records zero scalar, array, or aligned global allocations across the whole
sequence.

## Verification commands

```text
cmake -S lib/microworld-engine -B <build> -DMICROWORLD_ENGINE_BUILD_TESTS=ON
cmake --build <build> --config Release
ctest --test-dir <build> -C Release --output-on-failure

cmake -S lib/microworld/tests/consumer -B <consumer-build> ^
  -DMICROWORLD_STANDALONE_ENGINE_CONSUMER=ON
cmake --build <consumer-build> --config Release
<consumer-build>/Release/microworld_engine_consumer.exe

python lib/microworld/tools/CheckDependencyBoundaries.py ^
  --package Core=lib/microworld ^
  --package Memory=lib/microworld-memory ^
  --package Object=lib/microworld-object ^
  --package Engine=lib/microworld-engine ^
  --exclude build --exclude .pio --exclude __pycache__

python lib/microworld/tools/CheckClassDocumentation.py ^
  --root lib/microworld-engine --exclude build --exclude .pio ^
  --exclude __pycache__ --require-doxygen --max-sentences 3
```

Host behavior and compile evidence do not establish target runtime timing,
stack, heap, power, or physical-hardware behavior. No firmware upload, run,
or hardware measurement was performed.
