# MicroWorld Package

Inherits `../AGENTS.md`.

## Architecture

MicroWorld is a platform-neutral runtime kernel. `FApplication` guards a
consumer composition root, `TWorld<N>` dispatches registered Actors,
`TActor<N>` aggregates registered Components, `FNetwork` provides an
independent policy-free subsystem boundary, and `FTickFunction` owns scheduling
state shared by all tickable types.

## Core concepts

- Consumers own every concrete object; World and Actor registrations are
  bounded, non-owning, pointer-stable references.
- Lifecycle moves forward through constructed, playing, failed, or ended
  states. Begin failures become terminal and successful end is idempotent.
- One caller-supplied monotonic millisecond time source drives all scheduling.
- Actor, Component, and Network ticks remain independently configurable.
- Runtime failures return typed results; MicroWorld never logs, throws, reads
  hardware, or defines product policy.

## Implementation contract

- Use `F`/`T`/`E`/`b` naming and intent-focused Doxygen contracts for functions
  and persistent state as required by `lib/AGENTS.md`.
- Keep lifecycle and tick paths bounded, single-pass, allocation-free, and free
  of container mutation.
- No ESP-IDF, FreeRTOS, Arduino, radio, valve, or tutorial dependency may enter
  the production package.

## Verification

- Verify with `cmake -S lib/microworld -B lib/microworld/build`,
  `cmake --build lib/microworld/build`, and
  `ctest --test-dir lib/microworld/build --output-on-failure`.
- Run the declaration-documentation and directory-coverage tools, format all
  C/C++ files with the repository policy, and compile the downstream consumer
  probes appropriate to the installed toolchains.
