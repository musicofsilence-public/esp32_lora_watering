# Changelog

## 0.1.0 - 2026-07-18

Initial source release contract:

- `FApplication` guards a consumer-owned composition root and terminal begin
  failure.
- `TWorld<N>` and `TActor<N>` provide bounded non-owning registration,
  deterministic order, single ownership, and registration lock at play start.
- `FActorComponent` and `FNetwork` provide focused lifecycle/tick boundaries.
- `FTickConfiguration`, `FTickContext`, and `FTickFunction` provide independent
  configurable primary ticks, interval-zero updates, per-object delta,
  saturation, monotonic validation, and no catch-up bursts.
- CMake/CTest, PlatformIO consumer fixtures, host tests, fixed benchmarks,
  scoped validation tools, documentation, and a host example are included.

Initial verification:

- all 31 host behavior tests and the CTest entry pass;
- strict public-header, no-exceptions/no-RTTI, class-documentation, and
  folder-guide checks pass;
- C/C++ sources follow the repository `clang-format` policy, public functions
  and persistent state have intent-focused documentation, and every scoped
  contributor guide describes its module architecture and concepts;
- exact-version ESP32-S3 basic and benchmark consumers compile with PlatformIO
  Core 6.1.19, Espressif 32 platform 7.0.1, and ESP-IDF 6.0.1;
- the repository's existing ESP32-S3 firmware environment still compiles and
  no firmware was uploaded or run;
- the PlatformIO native consumer compiles with WinLibs GCC 16.1.0 on the user
  `PATH`, and its generated `program.exe` returns exit code zero.

Known limitations:

- source compatibility before 1.0 is not promised;
- runtime registration/removal, lookup, reflection, dynamic spawning, tick
  groups/prerequisites, parallel work, and product/platform policy are absent;
- ESP32-S3 benchmark runtime evidence remains blocked until explicit
  upload/hardware-run authorization and a clean exact source commit.
