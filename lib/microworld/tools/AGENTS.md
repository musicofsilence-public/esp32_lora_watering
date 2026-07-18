# Static Checks

Inherits `../AGENTS.md`.

## Architecture

Tools are read-only repository policy checks. The documentation scanner enforces
adjacent intent contracts for C++ type definitions, while the folder scanner
ensures every non-generated package directory has local architectural guidance.
Function and state documentation remains a declaration-level review requirement
because a regex-only parser must not pretend to understand arbitrary C++.
Production code never imports or invokes these scripts.

## Concepts

- Scans operate only under an explicit root.
- Generated build, PlatformIO, cache, and caller-specified directories are
  excluded so results describe maintained source.
- Diagnostics identify the file and declaration that violated policy.
- Scripts return non-zero on failure so CMake or CI can use them as gates.

## Documentation and verification

Every Python function and module-level policy constant needs a purpose-focused
docstring or comment. Keep scans deterministic and side-effect free. Verify with
`python lib/microworld/tools/CheckClassDocumentation.py --root lib/microworld --exclude build --exclude .pio --exclude __pycache__ --require-doxygen --max-sentences 3`
and
`python lib/microworld/tools/CheckFolderAgents.py --root lib/microworld --require-file AGENTS.md --require-file lib/AGENTS.md --exclude build --exclude .pio --exclude __pycache__`.
