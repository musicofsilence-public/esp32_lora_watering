# Architecture Decision Records

Inherits `../AGENTS.md`.

## Architecture

This directory records accepted MicroWorld design direction before code makes
that direction expensive to reverse. Each record owns one decision, its
constraints, rejected alternatives, consequences, and explicit revisit
triggers.

## Concepts

- `../../PROGRESS.md` owns current state; headers/tests/release metadata own
  released truth; result documents own measurements; package and budget docs
  own their respective build/map and resource facts; concept/roadmap own
  strategy and sequence.
- An accepted direction is not evidence that its API is implemented or
  released.
- A record distinguishes fixed invariants from provisional mechanisms.
- New evidence supersedes a decision through a later record; history remains
  readable.
- Records never invent target measurements, platform support, or product
  security properties.

## Verification

Cross-check status, dates, module names, dependency direction, and referenced
evidence against the approved roadmap, public headers, tests, and benchmark
records. Keep links relative and valid.
