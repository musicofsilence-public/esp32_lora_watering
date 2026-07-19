# MicroWorld Core Package

Inherits `../AGENTS.md`.

## Architecture

Core is the released platform-neutral lifecycle and tick package. `FApplication`
guards a consumer composition root, `TWorld<N>` registers Actors,
`TActor<N>` registers Components, and `FTickFunction` owns bounded scheduling.
Consumers own concrete objects; registration is non-owning and fixed-capacity.

Memory, Object, and Engine are adjacent packages above Core. The minimal
managed Engine is an accepted implementation candidate; simple fixed-capacity
Engine timers are next, not a broader framework.

## Concepts

- Keep lifecycle and tick paths bounded, single-pass, allocation-free, and
  free of structural mutation during dispatch.
- Use caller-supplied monotonic time and typed results. Core never logs,
  throws, reads hardware, or defines product policy.
- Keep vendor SDK, RTOS, radio, valve, and tutorial dependencies out.
- Preserve C++17, explicit ownership/failure, public API documentation,
  formatting, and behavior tests.
- `PROGRESS.md` is the sole live MicroWorld status record.

## Verification

Build Core, run its behavior tests and scoped checks, and compile relevant
downstream consumers after a boundary or public-contract change.
