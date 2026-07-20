# MicroWorld Package Family

## Product vision

MicroWorld is a mini engine for microcontrollers, shaped as a small UE5-like
runtime so UE5 game developers can carry familiar concepts into MCU work. It
helps make small applications, interactive software, and games easier to build
by providing a lightweight framework above platform and hardware interaction.

It is not a full UE5 clone. Keep only essential embedded-suitable features:
lifecycle, World, Actor, and Component concepts; optional bounded GC; smart
pointers; simple bounded networking through `FNetManager` and `INetDriver`; and
resource-efficient behavior. Portable code is intended to work with thin
platform adapters for ESP32, STM32, Raspberry Pi Pico/RP2040-class, and similar
MCUs; this is a product direction, not a claim that every target is supported.

This is the durable mission. The active roadmap still implements the smallest
usable milestone first and defers speculative systems until a real need exists.
`MICROWORLD_ROADMAP.md` (repository root) is that improvement plan and task
tracker; `lib/microworld/PROGRESS.md` remains the evidence record.

## Architecture

`lib/` contains standalone packages below the application composition roots.
Application, hardware, and product-policy code may depend on a library, but a
library must never depend back on those consumers. Each package owns its public
API, implementation, tests, examples, benchmarks, and release metadata.

## Concepts and boundaries

- Libraries use C++17 and expose deterministic ownership and lifetime rules.
- Consumers own concrete objects unless a package explicitly states otherwise.
- Platform adapters stay outside platform-neutral packages.
- Package tests and tools may depend on public APIs; production code never
  depends on tests, examples, benchmarks, or validation scripts.
- Implement the smallest usable milestone; do not add an abstraction or package
  until current code needs it.
- A directory inherits the nearest `AGENTS.md`. Add a local guide only when it
  introduces a distinct dependency boundary, ownership/lifecycle rule, or
  verification procedure; do not create one merely for directory coverage.

## Documentation and format

- Document every function declaration and every persistent, shared,
  configuration, or state variable with intent: why it exists, what invariant
  it preserves, or who owns it.
- Use descriptive names for local temporaries. Add a local comment only when
  the reason cannot be expressed clearly in code; never narrate an assignment.
- Format C/C++ files with the repository `clang-format` policy by passing it
  explicitly to `clang-format --style=file:clang-format`.
- Verify each library with its own build, tests, static checks, and examples.
