# Library Boundary

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

## Documentation and format

- Document every function declaration and every persistent, shared,
  configuration, or state variable with intent: why it exists, what invariant
  it preserves, or who owns it.
- Use descriptive names for local temporaries. Add a local comment only when
  the reason cannot be expressed clearly in code; never narrate an assignment.
- Format C/C++ files with the repository `clang-format` policy by passing it
  explicitly to `clang-format --style=file:clang-format`.
- Verify each library with its own build, tests, static checks, and examples.
