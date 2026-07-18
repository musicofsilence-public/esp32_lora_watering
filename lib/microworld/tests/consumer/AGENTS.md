# Downstream Consumer Probes

Inherits `../AGENTS.md`.

## Architecture

This is a downstream PlatformIO project, not part of the MicroWorld library.
It resolves the local 0.1.0 package through `symlink://`, then builds mutually
exclusive native, basic ESP32-S3, and executable benchmark applications.
MicroWorld never depends on these fixtures.

## Concepts

- The native environment verifies ordinary host consumption with exceptions
  and RTTI disabled.
- The basic ESP32-S3 environment proves the public package crosses the
  ESP-IDF/toolchain boundary without platform headers entering MicroWorld.
- The benchmark environment adds target-only measurement code around the same
  public scheduling API.
- PlatformIO source filtering selects the native entry point; the ESP-IDF
  component CMake file selects exactly one `app_main` from the environment's
  isolated build directory.
- Both ESP32 environments inherit repository N16R8 flash, PSRAM, and partition
  defaults but never upload unless separately authorized.

## Documentation and verification

Document each environment-specific function and persistent measurement value by
its probe purpose. Verify compilation with explicit `-e` environments; a native
probe on Windows requires GNU `g++` on `PATH`.
